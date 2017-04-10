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

#ifndef SELF_CLICKER_DATA_H
#define SELF_CLICKER_DATA_H

#include "sensors/IData.h"
#include "SelfLib.h"


class ClickerData : public IData
{
public:
    RTTI_DECL();

    //! Construction
    ClickerData(const std::string & a_Input) : m_Input( a_Input )
    {}

    ClickerData() : m_Input( EMPTY_STRING )
    {}

    //! ISerializable interface
    virtual void Serialize(Json::Value & json)
    {
        json["m_Input"] = m_Input;
    }
    virtual void Deserialize(const Json::Value & json)
    {
        m_Input = json["m_Input"].asString();
    }

    //!Accessors
    const std::string & GetInput() const
    {
        return m_Input;
    }

private:
    //!Data
    std::string			m_Input;
};

#endif //SELF_CLICKER_DATA_H
