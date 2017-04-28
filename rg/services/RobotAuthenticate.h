// Copyright (c) 2011 rubicon IT GmbH
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

