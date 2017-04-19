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
