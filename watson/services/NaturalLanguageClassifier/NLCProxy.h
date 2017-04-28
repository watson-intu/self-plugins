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


#ifndef WDC_NLC_PROXY_H
#define WDC_NLC_PROXY_H

#include "classifiers/TextClassifier.h"
#include "blackboard/Text.h"

class NaturalLanguageClassifier;
struct Classifiers;
struct Classifier;

class NLCProxy : public ITextClassifierProxy
{
public:
    RTTI_DECL();

	//! Construction
	NLCProxy();
	virtual ~NLCProxy();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

    //! ITextClassifierProxy interface
	virtual void Start();
	virtual void Stop();

	virtual void ClassifyText( Text::SP a_spText, Delegate<ClassifyResult *> a_Callback );
    
	//! Types

	//! Classifier training and management functions
	const std::string & GetClassifierId() const
	{
		return m_ClassifierId;
	}

	template<typename T>
	boost::shared_ptr<T> FindFilter() const
	{
		for (size_t i = 0; i < m_Filters.size(); ++i)
		{
			boost::shared_ptr<T> spFilter = DynamicCast<T>( m_Filters[i] );
			if (spFilter)
				return spFilter;
		}
		return boost::shared_ptr<T>();
	}

	// queue data to train our classifier with a new phrase and class.
	bool RetrainClassifier(const std::string & a_Text, const std::string & a_Class);
	void CheckClassifiers();
	void OnGetClassifiers( Classifiers * a_pClassifiers );
	void OnClassifierDeleted(const Json::Value & json);
	void OnClassifierTrained( Classifier * a_pClassifer );

private:
	//! Data
	NaturalLanguageClassifier *		m_pNLC;
	std::string						m_ClassifierFile;				// training file to upload/update
	std::string						m_ClassifierId;					// intent classifier to use
	std::string						m_Language;
	bool							m_bCheckedClassifiers;			// true once we've checked our classifiers

	TimerPool::ITimer::SP			m_spClassifiersTimer;			// timer use to check our classifiers

	//! Helper request object
	class Request
	{
    public:
        Request( NLCProxy * a_pNLCProxy, Text::SP a_spText, Delegate<ClassifyResult *> a_Callback );

		void OnTextClassified(const Json::Value & json);

    private:
        NLCProxy *              	        m_pProxy;
        Text::SP                         	m_spText;
        Delegate<ClassifyResult *>     		m_Callback;
	};
};

#endif // WDC_NLC_PROXY_H