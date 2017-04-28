// Copyright (c) 2011 rubicon IT GmbH
#ifndef RG_ROBOT_MAIL_H
#define RG_ROBOT_MAIL_H

#include "services/IMail.h"

class RobotMail : public IMail
{
public:
	RTTI_DECL();

	RobotMail() : IMail( "RobotGatewayV1", AUTH_NONE )
	{}

	//! IMail interface
	void SendEmail( const std::string & a_To, 
		const std::string & a_Subject, 
		const std::string & a_Message, 
		Delegate<Request *> a_Callback );
};

#endif
