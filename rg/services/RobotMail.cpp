// Copyright (c) 2011 rubicon IT GmbH
#include "RobotMail.h"
#include "RobotGateway.h"
#include "SelfInstance.h"

RTTI_IMPL( RobotMail, IMail );
REG_SERIALIZABLE( RobotMail );
REG_OVERRIDE_SERIALIZABLE(IMail,RobotMail);

void RobotMail::SendEmail( const std::string & a_To, 
	const std::string & a_Subject, 
	const std::string & a_Message, 
	Delegate<Request *> a_Callback )
{
	RobotGateway * pGateway = SelfInstance::GetInstance()->FindService<RobotGateway>();
	if ( pGateway != NULL )
	{
		Headers headers;
		headers["Content-Type"] = "text/plain";
		headers["subject"] = a_Subject;

		if ( a_To.size() > 0 )
			headers["to"] = a_To;
		else if ( pGateway->m_OrgAdminList.size() > 0 )
			headers["to"] = pGateway->m_OrgAdminList;

		if ( headers.find( "to" ) != headers.end() )
			new Request( pGateway, "/v1/communications/sendEmail", "POST", headers, a_Message, a_Callback);
	}
	else
	{
		Log::Error( "RobotMail", "RobotGateway service is required." );
		a_Callback( NULL );
	}
}

