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

#ifndef SELF_MUTEAGENT_H
#define SELF_MUTEAGENT_H

#include "agent/IAgent.h"
#include "blackboard/IThing.h"
#include "sensors/SensorManager.h"

class MuteAgent : public IAgent
{
public:
    RTTI_DECL();

    MuteAgent() : m_IsMuted( false )
    {}

    //! ISerializable interface
    void Serialize( Json::Value & json );
    void Deserialize( const Json::Value & json );

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();

private:

    //! Event Handlers
    void OnClick(const ThingEvent & a_ThingEvent);

    //! Data
    bool m_IsMuted;
};

#endif //SELF_ASIMOVAGENT_H
