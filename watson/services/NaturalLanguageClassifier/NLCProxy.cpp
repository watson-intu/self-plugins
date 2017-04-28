/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#include "NLCProxy.h"
#include "NaturalLanguageClassifier.h"
#include "utils/Path.h"

#include "SelfInstance.h"

#include <fstream>

REG_SERIALIZABLE( NLCProxy );
RTTI_IMPL( NLCProxy, ITextClassifierProxy );

#define ENABLE_DEBUGGING					0
#define ENABLE_CLASSIFIER_DELETE			1

#pragma warning(disable:4996)

//! how often to check our classifiers
const double CHECK_CLASSIFIERS_INTERVAL = 300.0f;


NLCProxy::NLCProxy() :
	m_pNLC( NULL ),
	m_bCheckedClassifiers(false)
{}

NLCProxy::~NLCProxy()
{}

void NLCProxy::Serialize(Json::Value & json)
{
	ITextClassifierProxy::Serialize( json );

	json["m_ClassifierFile"] = m_ClassifierFile;
	json["m_ClassifierId"] = m_ClassifierId;
    json["m_Language"] = m_Language;

	if ( m_Language.size() == 0 )
		m_Language = "en";
}

void NLCProxy::Deserialize(const Json::Value & json)
{
	ITextClassifierProxy::Deserialize( json );

	if (json.isMember("m_ClassifierFile"))
		m_ClassifierFile = json["m_ClassifierFile"].asString();
	if (json.isMember("m_ClassifierId"))
		m_ClassifierId = json["m_ClassifierId"].asString();
	if (json.isMember("m_Language"))
		m_Language = json["m_Language"].asString();

}

void NLCProxy::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	m_pNLC = pInstance->FindService<NaturalLanguageClassifier>();
	if (m_pNLC == NULL )
		Log::Error("NLCProxy", "NaturalLanguageClassifier service not available.");

	CheckClassifiers();

	Log::Status("NLCProxy", "NLCProxy started for %s", m_ClassifierFile.c_str() );
}

void NLCProxy::Stop()
{}

void NLCProxy::ClassifyText( Text::SP a_spText, Delegate<ClassifyResult *> a_Callback )
{
	new Request( this, a_spText, a_Callback );
}

bool NLCProxy::RetrainClassifier(const std::string & a_Text,
	const std::string & a_Class)
{
	if (m_ClassifierFile.size() == 0)
		m_ClassifierFile = "shared/self_nlc.csv";
	const std::string & staticPath = Config::Instance()->GetStaticDataPath();
	std::string classifierFile( staticPath + m_ClassifierFile );

	// append new data onto the file, then
	std::string cleanText(a_Text);
	StringUtil::Replace(cleanText, ",", "");
	StringUtil::Replace(cleanText, "?", "");
	StringUtil::Replace(cleanText, "!", "");
	StringUtil::Replace(cleanText, ".", "");

	std::ofstream output(classifierFile.c_str(), std::ios::in | std::ios::out | std::ios::ate | std::ios::binary);
	if (!output.is_open())
	{
		Log::Error("NaturalLanguageClassifier", "Failed to open training file %s.", classifierFile.c_str());
		return false;
	}

	output << "\r\n" << cleanText << "," << a_Class;
	output.close();

	return true;
}

void NLCProxy::CheckClassifiers()
{
	const std::string & staticPath = Config::Instance()->GetStaticDataPath();

	if (m_ClassifierFile.size() > 0
		&& Time::GetFileModifyTime(staticPath + m_ClassifierFile) != 0)
	{
		if ( m_pNLC->IsConfigured() )
		{
			m_pNLC->FindClassifiers(Path(m_ClassifierFile).GetFile(),
				DELEGATE(NLCProxy, OnGetClassifiers, Classifiers *, this));
		}
		else
			Log::Warning( "NLCProxy", "NLC is not configured." );

		// start a timer to keep checking classifiers on a regular interval, we'll switch automatically
		// to a new classifier when it becomes available.
		if (!m_spClassifiersTimer)
		{
			m_spClassifiersTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(NLCProxy, CheckClassifiers, this),
				CHECK_CLASSIFIERS_INTERVAL, true, true);
		}
	}
}

void NLCProxy::OnGetClassifiers(Classifiers * a_pClassifiers)
{
	if (a_pClassifiers != NULL)
	{
		const std::string & staticPath = Config::Instance()->GetStaticDataPath();
		std::string classifierFile( staticPath + m_ClassifierFile );
		time_t classifierFileTime = Time::GetFileModifyTime(classifierFile);

		m_ClassifierId.clear();
		time_t newestClassifierTime = 0;
		time_t availClassifierTime = 0;

		std::string classifierName(Path(m_ClassifierFile).GetFile());
		std::map<std::string, time_t> classifierTimes;

		const std::vector<Classifier> & classifiers = a_pClassifiers->m_Classifiers;
		for (int i = 0; i<(int)classifiers.size(); ++i)
		{
			const Classifier & classifier = classifiers[i];

			struct tm t;
			memset(&t, 0, sizeof(t));

			// 2016-05-27T05:43:25.828Z
			sscanf(classifier.m_Created.c_str(), "%d-%d-%dT%d:%d:%d",
				&t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec);
			t.tm_year -= 1900;
			t.tm_mon -= 1;		// 0 - 11

			time_t classifierTime = mktime(&t);
			if (classifierTime < 0)
				continue;
			classifierTimes[classifier.m_ClassifierId] = classifierTime;		// save for purge check

			if (classifierTime > newestClassifierTime)
				newestClassifierTime = classifierTime;

			if (classifier.m_Status == "Available"
				&& classifierTime > availClassifierTime)
			{
				availClassifierTime = classifierTime;
				m_ClassifierId = classifier.m_ClassifierId;
			}
		}

		if (m_ClassifierId.size() > 0)
			Log::Status("NLCProxy", "Selected classifier %s", m_ClassifierId.c_str());

		// if we have no classifier (even those training) newer than our current file stamp, then
		// go ahead and train the new classifier.
		if (newestClassifierTime < classifierFileTime)
		{
			Log::Status("QuestionAgent", "Training new classifier %s using file %s.", classifierName.c_str(), classifierFile.c_str());
			if (! m_pNLC->TrainClassifierFile(classifierName, m_Language, classifierFile,
				DELEGATE(NLCProxy, OnClassifierTrained, Classifier *, this)))
			{
				Log::Error("QuestionAgent", "Failed to train classifier.");
			}
		}

		// purge old classifiers
		for (int i = 0; i<(int)classifiers.size(); ++i)
		{
			const Classifier & classifier = classifiers[i];
			if (classifier.m_ClassifierId == m_ClassifierId)
				continue;		// our current classifier
			if (classifierTimes.find(classifier.m_ClassifierId) == classifierTimes.end())
				continue;
			time_t classifierTime = classifierTimes[classifier.m_ClassifierId];
			if (classifierTime < availClassifierTime &&
				(availClassifierTime - classifierTime) > CHECK_CLASSIFIERS_INTERVAL )
			{
				Log::Status("QuestionAgent", "Deleting classifier %s", classifier.m_ClassifierId.c_str());
				m_pNLC->DeleteClassifer(classifier.m_ClassifierId,
					DELEGATE(NLCProxy, OnClassifierDeleted, const Json::Value &, this));
			}
		}

		delete a_pClassifiers;
	}
	else
	{
		Log::Error("QuestionAgent", "Failed to get classifiers.");
	}

	m_bCheckedClassifiers = true;
}

void NLCProxy::OnClassifierTrained( Classifier * a_pClassifer )
{
	if (a_pClassifer != NULL)
		Log::Status("NLCProxy", "New classifier trained: %s", a_pClassifer->m_ClassifierId.c_str());
	else
		Log::Error("NLCProxy", "Failed to train classifier.");
}

void NLCProxy::OnClassifierDeleted(const Json::Value & json)
{
	if (!json.isNull())
		Log::Status("TextClassifer", "Classifier deleted: %s", json.toStyledString().c_str());
	else
		Log::Error("NLCProxy", "Failed to delete classifier.");
}

//---------------------------------------------------------

//! Helper object for handling specific request
NLCProxy::Request::Request(NLCProxy * a_pProxy, Text::SP a_spText, Delegate<ClassifyResult *> a_Callback) : 
	m_pProxy( a_pProxy ),
	m_spText( a_spText ),
	m_Callback( a_Callback )
{
	if ( m_pProxy->m_pNLC != NULL && m_pProxy->m_pNLC->IsConfigured() )
		m_pProxy->m_pNLC->Classify(m_pProxy->m_ClassifierId, m_spText->GetText(),
			DELEGATE(Request, OnTextClassified, const Json::Value &, this));
	else
		OnTextClassified( Json::Value() );
}

void NLCProxy::Request::OnTextClassified(const Json::Value & a_Result )
{
#if ENABLE_DEBUGGING
	Log::Debug( "NLCProxy", "m_Intent = %s", a_Result.toStyledString().c_str() );
#endif

	ClassifyResult * pResult = new ClassifyResult();
	pResult->m_Result = a_Result;

	if (! pResult->m_Result.isNull() )
	{
		bool bIgnore = false;
		for (size_t i = 0; i < m_pProxy->m_Filters.size() && !bIgnore; ++i)
			bIgnore |= m_pProxy->m_Filters[i]->ApplyFilter(pResult->m_Result);

		if (! bIgnore )
		{
			pResult->m_TopClass = pResult->m_Result["top_class"].asString();
			pResult->m_fConfidence = pResult->m_Result["classes"][0]["confidence"].asDouble();
			pResult->m_bPriority = m_pProxy->m_bPriority;
		}
		else
		{
			Log::Debug("NLCProxy", "Ignoring filtered intent: %s, TextId: %p", 
				pResult->m_Result["top_class"].asCString(), m_spText.get() );
			pResult->m_TopClass = "ignored";
		}
	}
	else
	{
		Log::Error( "NLCProxy", "ClassifyText Failed - Text: %s, TextId: %p",
			m_spText->GetText().c_str(), m_spText.get() );
		pResult->m_TopClass = "failure";
	}
	
	// If valid, hit callback to text classifier
    if ( m_Callback.IsValid() )
        m_Callback( pResult );

	delete this;
}

