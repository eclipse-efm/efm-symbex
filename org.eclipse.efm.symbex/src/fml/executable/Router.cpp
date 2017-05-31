/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#include "Router.h"

#include <fml/expression/AvmCode.h>

#include <fml/executable/ExecutableForm.h>
#include <fml/executable/InstanceOfMachine.h>
#include <fml/executable/InstanceOfPort.h>


namespace sep
{


/*
 * STATIC ATTRIBUTES
 */
Router Router::_NULL_;


/**
 * SETTER
 * mInputRoutingTable
 */
void Router::appendInputRouting(InstanceOfPort * aPortInstance,
		const RoutingData & aRoutingData)
{
	RoutingData & oldRoutingData =
			getInputRouting( aPortInstance->getRouteOffset() );
	if( oldRoutingData.valid() )
	{
		if( oldRoutingData.getMID() == aRoutingData.getMID() )
		{
			oldRoutingData.getBufferInstance().append(
					aRoutingData.getBufferInstance() );

			oldRoutingData.getBufferInstance().makeUnique();
		}
		else
		{
			AVM_OS_WARNING_ALERT
					<< "Unexpected a port < "
					<< aPortInstance->getFullyQualifiedNameID()
					<< " > with another routing<input> data in << "
					<< getMachine()->getFullyQualifiedNameID() << " >> !!!"
					<< SEND_ALERT;

			AVM_OS_LOG << "Input Routing Data of port : "
					<< aPortInstance->getFullyQualifiedNameID() << std::endl;
			oldRoutingData.toStream(AVM_OS_LOG << AVM_TAB_INDENT);
			AVM_OS_LOG << END_INDENT << std::endl;
		}
	}
	else
	{
		setInputRouting(aPortInstance->getRouteOffset(), aRoutingData);
	}
}

void Router::setInputRouting(InstanceOfPort * aPortInstance,
		const RoutingData & aRoutingData) const
{
	if( getInputRoutingTable().get(aPortInstance->getRouteOffset()).valid() )
	{
		AVM_OS_WARNING_ALERT
				<< "Unexpected a port < "
				<< aPortInstance->getFullyQualifiedNameID()
				<< " > with another routing<input> data in << "
				<< getMachine()->getFullyQualifiedNameID() + " >> !!!"
				<< SEND_ALERT;

		AVM_OS_LOG << "Input Routing Data of port : "
				<< aPortInstance->getFullyQualifiedNameID() << std::endl;

		getInputRoutingTable().get(aPortInstance->getRouteOffset()).
				toStream(AVM_OS_LOG << AVM_TAB_INDENT);
		AVM_OS_LOG << END_INDENT << std::endl;
	}

	setInputRouting(aPortInstance->getRouteOffset(), aRoutingData);
}


/**
 * GETTER - SETTER
 * mOutputRoutingTable
 */
void Router::appendOutputRouting(InstanceOfPort * aPortInstance,
		const RoutingData & aRoutingData)
{
	RoutingData & oldRoutingData =
			getOutputRouting( aPortInstance->getRouteOffset() );
	if( oldRoutingData.valid() )
	{
		if( oldRoutingData.getMID() == aRoutingData.getMID() )
		{
			oldRoutingData.getBufferInstance().append(
					aRoutingData.getBufferInstance() );

			oldRoutingData.getBufferInstance().makeUnique();
		}
		else
		{
			AVM_OS_WARNING_ALERT
					<< "Unexpected a port < "
					<< aPortInstance->getFullyQualifiedNameID()
					<< " > with another routing<output> data in << "
					<< getMachine()->getFullyQualifiedNameID() << " >> !!!"
					<< SEND_ALERT;

			AVM_OS_LOG << "Output Routing Data of port : "
					<< aPortInstance->getFullyQualifiedNameID() << std::endl;
			oldRoutingData.toStream(AVM_OS_LOG << AVM_TAB_INDENT);

			AVM_OS_LOG << END_INDENT << std::endl;
		}
	}
	else
	{
		setOutputRouting(aPortInstance->getRouteOffset(), aRoutingData);
	}
}

void Router::setOutputRouting(InstanceOfPort * aPortInstance,
		const RoutingData & aRoutingData) const
{
	if( getOutputRoutingTable().get(aPortInstance->getRouteOffset()).valid() )
	{
		AVM_OS_WARNING_ALERT
				<< "Unexpected a port < "
				<< aPortInstance->getFullyQualifiedNameID()
				<< " > with another routing<output> data in << "
				<< getMachine()->getFullyQualifiedNameID() << " >> !!!"
				<< SEND_ALERT;

		AVM_OS_LOG << "Output Routing Data of port : "
				<< aPortInstance->getFullyQualifiedNameID() << std::endl;

		getOutputRoutingTable().get(aPortInstance->getRouteOffset()).
				toStream(AVM_OS_LOG << AVM_TAB_INDENT);

		AVM_OS_LOG << END_INDENT << std::endl;
	}

	setOutputRouting(aPortInstance->getRouteOffset(), aRoutingData);
}


/**
 * TESTER
 */
bool Router::hasInputRouting(InstanceOfPort * aPort) const
{
	return( aPort->getModifier().hasDirectionInput()
		&& (aPort->getRouteOffset() < getInputRoutingTable().size())
		&& (getInputRouting(aPort->getRouteOffset()).getPort() == aPort) );
}

bool Router::hasOutputRouting(InstanceOfPort * aPort) const
{
	return( aPort->getModifier().hasDirectionOutput()
		&& (aPort->getRouteOffset() < getOutputRoutingTable().size())
		&& (getOutputRouting(aPort->getRouteOffset()).getPort() == aPort) );
}


/**
 * Serialization
 */
void RouterElement::toStream(OutStream & os) const
{
	os << TAB << "router " << mMachine->getFullyQualifiedNameID();
	if( mMachine->isThis() )
	{
		os << "< " << mMachine->getExecutable()->getFullyQualifiedNameID()
			<< " >";
	}
	AVM_DEBUG_REF_COUNTER(os);
	os << " {" << EOL;

	// routing table for INSTANCE
	if( mInputRoutingTable.nonempty() )
	{
		os << TAB << "input:" << EOL_INCR_INDENT;

		TableOfRoutingData::const_iterator it = mInputRoutingTable.begin();
		TableOfRoutingData::const_iterator endIt = mInputRoutingTable.end();
		for( ; it != endIt ; ++it )
		{
			if( (*it).valid() )
			{
				(*it).toStream(os);
			}
			else
			{
				os << TAB << "RoutingData<null>" << EOL;
			}
		}
		os << DECR_INDENT;
	}

	if( mOutputRoutingTable.nonempty() )
	{
		os << TAB << "output:" << EOL_INCR_INDENT;

		TableOfRoutingData::const_iterator it = mOutputRoutingTable.begin();
		TableOfRoutingData::const_iterator endIt = mOutputRoutingTable.end();
		for( ; it != endIt ; ++it )
		{
			if( (*it).valid() )
			{
				(*it).toStream(os);
			}
			else
			{
				os << TAB << "RoutingData<null>" << EOL;
			}
		}
		os << DECR_INDENT;
	}

	os << TAB << "}" << EOL << std::flush;
}


}
