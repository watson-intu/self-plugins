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

#ifndef RG_ROBOT_AUTHENTICATE_H
#define RG_ROBOT_AUTHENTICATE_H

#include "services/IAuthenticate.h"

class RobotAuthenticate : public IAuthenticate
{
public:
	RTTI_DECL();

	RobotAuthenticate() : IAuthenticate( "RobotGatewayV1", AUTH_NONE )
	{}

	//! IAuthenticate interface
	void Authenticate( const std::string & a_EmodimentToken, 
		Delegate<const Json::Value &> a_Callback );
};

#endif

