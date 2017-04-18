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

#include "services/IMail.h"
#include "RobotGateway.h"
#include "SelfInstance.h"

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
};

RTTI_IMPL( RobotMail, IMail );
REG_SERIALIZABLE( RobotMail );

