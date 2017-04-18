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

#ifndef SELF_INTERACT_H
#define SELF_INTERACT_H

#include "utils/IService.h"

//! Service implementation for the Interact service (Project Sagan)
class Interact : public IService
{
public:
	RTTI_DECL();

	//! Construction
	Interact();

	//! ISerializable
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();

	//! Send conversation to the interact service, it will invoke the provided
	//! callback with the response.
	void Converse(const std::string & a_Input, Delegate<const Json::Value &> a_Callback);
};

#endif	// SELF_INTERACT_H
