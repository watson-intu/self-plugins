/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */

#include "WEXIntent.h"

REG_SERIALIZABLE(WEXIntent);
RTTI_IMPL( WEXIntent, IIntent );

void WEXIntent::Serialize(Json::Value &json)
{
    IIntent::Serialize(json);

    json["m_Text"] = m_Text;
    json["m_Collection"] = m_Collection;
}

void WEXIntent::Deserialize(const Json::Value &json)
{
    IIntent::Deserialize(json);

    m_Text = json["m_Text"].asString();
    m_Collection = json["m_Collection"].asString();
}

void WEXIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
    if ( a_Intent.isMember("text") )
        m_Text = a_Intent["text"].asString();
    if ( a_Intent.isMember("goal_params") )
    {
        m_GoalParams = a_Intent["goal_params"];
        m_Collection = a_Intent["goal_params"]["answer"]["response"][0].asString();
    }
}

