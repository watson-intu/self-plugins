// Copyright (c) 2011 rubicon IT GmbH
#include "RobotAuthenticate.h"
#include "RobotGateway.h"
#include "SelfInstance.h"

RTTI_IMPL( RobotAuthenticate, IAuthenticate );
REG_SERIALIZABLE( RobotAuthenticate );
REG_OVERRIDE_SERIALIZABLE( IAuthenticate, RobotAuthenticate );

void RobotAuthenticate::Authenticate( const std::string & a_EmodimentToken, 
	Delegate<const Json::Value &> a_Callback )
{
	RobotGateway * pGateway = SelfInstance::GetInstance()->FindService<RobotGateway>();
	if ( pGateway != NULL )
	{
		Headers headers;
		headers["EmbodimentAuthorization"] = a_EmodimentToken;

		new RequestJson( pGateway, "/v1/membership/authenticateEmbodimentsAgainstOrg", "GET",
			headers, EMPTY_STRING, a_Callback );
	}
	else
	{
		Log::Error( "RobotAuthenticate", "RobotGateway service is required." );
		a_Callback( Json::Value() );
	}
}


