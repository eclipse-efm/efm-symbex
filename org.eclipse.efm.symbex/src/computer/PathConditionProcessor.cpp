/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 21 juin 2010
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#include "PathConditionProcessor.h"

#include <common/BF.h>

#include <computer/primitive/AvmPrimitiveProcessor.h>

#include <fml/expression/AvmCode.h>
#include <fml/expression/ExpressionSimplifier.h>

#include <fml/operator/OperatorManager.h>

#include <solver/api/SolverFactory.h>



namespace sep
{

/*
 * ATTRIBUTE
 */
// true:> Utilise les SAT-SOLVER pour checker les conditions
bool PathConditionProcessor::CHECK_SATISFIABILITY_WITH_SATSOLVER_ENABLED = false;

bool PathConditionProcessor::STRONGLY_CHECK_SATISFIABILITY_WITH_SATSOLVER_ENABLED = false;

// false:> Pas de separation sur les OR dans les PCs
bool PathConditionProcessor::PATH_CONDIDITON_DISJONCTION_SEPARATION_ENABLED = false;


// false:> Pas de separation sur les OR dans les PCs
bool PathConditionProcessor::NODE_CONDITION_COMPUTATION_ENABLED = false;


////////////////////////////////////////////////////////////////////////////////
///// CHECK SATISFIABILITY
////////////////////////////////////////////////////////////////////////////////

bool PathConditionProcessor::isWeakSatisfiable(const BF & aCondition)
{
	return( SolverFactory::isWeakSatisfiable(aCondition,
			PathConditionProcessor::CHECK_SATISFIABILITY_WITH_SATSOLVER_ENABLED) );
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///// FIRED CONDITION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool PathConditionProcessor::setNodeCondition(
		ExecutionData & apED, const BF & theCondition)
{
	apED.makeWritable();

//	RuntimeForm * aRF = apED.makeWritableRuntimeForm( apED.getRID() );

	apED.setNodeCondition( theCondition );

//	aRF->setNodeCondition( theCondition );

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "SET FIRED CONDITION :"
			<< str_indent( theCondition ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

	return( true );
}


bool PathConditionProcessor::addNodeCondition(
		ExecutionData & apED, const BF & theCondition)
{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "ADD FIRED CONDITION :begin>"
			<< str_indent( theCondition ) << " with FC:"
			<< str_indent( apED.getNodeCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

	if( theCondition.isEqualTrue() )
	{
		//!!! NOTHING
	}
	else if( theCondition.isEqualFalse() ||
			apED.getNodeCondition().isEqualFalse() )
	{
		return( false );
	}
	else
	{
		apED.makeWritable();

//		RuntimeForm * aRF = apED.makeWritableRuntimeForm( apED.getRID() );

		if( apED.getNodeCondition().isEqualTrue() )
		{
//			aRF->setNodeCondition( theCondition );

			apED.setNodeCondition( theCondition );

		}
		else
		{
//			aRF->setNodeCondition(
//					ExpressionSimplifier::simplif_and(
//							aRF->getNodeCondition(), theCondition) );

			apED.setNodeCondition(
					ExpressionSimplifier::simplif_and(
							apED.getNodeCondition(), theCondition) );
		}
	}

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "ADD FIRED CONDITION :end>"
			<< str_indent( apED.getNodeCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

	return( true );
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///// FIRED TIMED CONDITION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool PathConditionProcessor::setNodeTimedCondition(
		ExecutionData & apED, const BF & theTimedCondition)
{
	apED.makeWritable();

//	RuntimeForm * aRF = apED.makeWritableRuntimeForm( apED.getRID() );

	apED.setNodeTimedCondition( theTimedCondition );

//	aRF->setNodeTimedCondition( theTimedCondition );

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "SET FIRED TIMED CONDITION :"
			<< str_indent( theTimedCondition ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

	return( true );
}


bool PathConditionProcessor::addNodeTimedCondition(
		ExecutionData & apED, const BF & theTimedCondition)
{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "ADD FIRED TIMED CONDITION :begin>"
			<< str_indent( theTimedCondition ) << " with FtC:"
			<< str_indent( apED.getNodeTimedCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

	if( theTimedCondition.isEqualTrue() )
	{
		//!!! NOTHING
	}
	else if( theTimedCondition.isEqualFalse() ||
			apED.getNodeTimedCondition().isEqualFalse() )
	{
		return( false );
	}
	else
	{
		apED.makeWritable();

//		RuntimeForm * aRF = apED.makeWritableRuntimeForm( apED.getRID() );

		if( apED.getNodeTimedCondition().isEqualTrue() )
		{
//			aRF->setNodeTimedCondition( theTimedCondition );

			apED.setNodeTimedCondition( theTimedCondition );

		}
		else
		{
//			aRF->setNodeTimedCondition(
//					ExpressionSimplifier::simplif_and(
//							aRF->getNodeTimedCondition(), theTimedCondition) );

			apED.setNodeTimedCondition(
					ExpressionSimplifier::simplif_and(
							apED.getNodeTimedCondition(), theTimedCondition) );
		}
	}

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "ADD FIRED TIMED CONDITION :end>"
			<< str_indent( apED.getNodeTimedCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

	return( true );
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///// PATH CONDITION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool PathConditionProcessor::addPathCondition(ExecutionData & apED,
		const BF & theCondition, bool considerFiredConditon)
{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "ADD PATH CONDITION :begin>"
			<< str_indent( theCondition ) << " with PC:"
			<< str_indent( apED.getPathCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )


	if( theCondition.isEqualFalse() || apED.getPathCondition().isEqualFalse() )
	{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << "ADD PATH CONDITION :end> false" << std::endl;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( false );
	}

	else if( not theCondition.isEqualTrue() )
	{
		BF thePathCondition = ( apED.getPathCondition().isEqualTrue() ) ?
				theCondition : ExpressionSimplifier::simplif_and(
						apED.getPathCondition(), theCondition);

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "ADD PATH CONDITION :isSat?>"
			<< str_indent( thePathCondition ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		if( PathConditionProcessor::isWeakSatisfiable(thePathCondition) )
		{
			apED.makeWritable();

			if( considerFiredConditon
				&& ( NODE_CONDITION_COMPUTATION_ENABLED
					|| apED.isEnabledNodeCondition() ) )
			{
				if( not addNodeCondition(apED, theCondition) )
				{
					return( false );
				}
			}

			apED.setPathCondition( thePathCondition );
		}
		else
		{
//			apED.setPathCondition( ExpressionConstant::BOOLEAN_FALSE );

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << "ADD PATH CONDITION :end> false" << std::endl;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

			return( false );
		}
	}

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "ADD PATH CONDITION :end>"
			<< str_indent( apED.getPathCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

	return( true );
}


bool PathConditionProcessor::appendPathCondition(
		ExecutionData & apED, BF & theCondition,
		CollectionOfExecutionData & listOfOutputED)
{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "APPEND PATH CONDITION :begin>"
			<< str_indent( theCondition ) << " with PC:"
			<< str_indent( apED.getPathCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )


	if( theCondition.isEqualFalse() || apED.getPathCondition().isEqualFalse() )
	{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << "APPEND PATH CONDITION : false" << std::endl;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( false );
	}

	else if( theCondition.isEqualTrue() )
	{
		listOfOutputED.append( apED );

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "APPEND PATH CONDITION :end>"
			<< str_indent( apED.getPathCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( true );
	}

	else if( PATH_CONDIDITON_DISJONCTION_SEPARATION_ENABLED &&
			theCondition.is< AvmCode >() &&
			theCondition.to< AvmCode >().isOpCode( AVM_OPCODE_OR ) )
	{
		ExecutionData newED;
		std::size_t count = 0;

		if( apED.getPathCondition().isEqualTrue() )
		{
			for( const auto & itOperand :
				theCondition.to< AvmCode >().getOperands() )
			{
				newED = apED;
				newED.makeWritable();

				if( setCondition(newED, itOperand, itOperand) )
				{
					listOfOutputED.append( newED );
					++count;
				}
			}
		}

		else
		{
			BF thePathCondition;

			for( const auto & itOperand :
				theCondition.to< AvmCode >().getOperands() )
			{
				newED = apED;
				newED.makeWritable();

				thePathCondition = ExpressionSimplifier::simplif_and(
						newED.getPathCondition(), itOperand);
				if( setCondition(newED, itOperand, thePathCondition) )
				{
					listOfOutputED.append( newED );
				}
			}
		}

		return( count > 0 );
	}
	else
	{
		BF thePathCondition;
		if( apED.getPathCondition().isEqualTrue() )
		{
			thePathCondition = theCondition;
		}
		else
		{
			thePathCondition = ExpressionSimplifier::simplif_and(
					apED.getPathCondition(), theCondition);
		}

		if( setCondition(apED, theCondition, thePathCondition) )
		{
			listOfOutputED.append( apED );

			return( true );
		}

		return( false );
	}
}


bool PathConditionProcessor::setCondition(
		ExecutionData & apED, const BF & theNodeCondition,
		const BF & thePathCondition, bool considerFiredConditon)
{
	// Verification de la satisfiabilite des gardes
	// compte tenu du parametrage dans favm
	if( PathConditionProcessor::isWeakSatisfiable(thePathCondition) )
	{
		apED.makeWritable();

		if( considerFiredConditon
			&&  ( NODE_CONDITION_COMPUTATION_ENABLED
				|| apED.isEnabledNodeCondition() ) )
		{
			if( not addNodeCondition(apED, theNodeCondition) )
			{
				return( false );
			}
		}

		apED.setPathCondition( thePathCondition );

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "SET PATH CONDITION :end>"
			<< str_indent( thePathCondition ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( true );
	}
	else
	{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << "SET PATH CONDITION :end> false" << std::endl;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( false );
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///// TIMED CONDITION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool PathConditionProcessor::addPathTimedCondition(
		ExecutionData & apED, const BF & theTimedCondition)
{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "APPEND PATH TIMED CONDITION :begin>"
			<< str_indent( theTimedCondition ) << " with PC: "
			<< str_indent( apED.getPathTimedCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )


	if( theTimedCondition.isEqualFalse() ||
		apED.getPathTimedCondition().isEqualFalse() )
	{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << "APPEND PATH TIMED CONDITION :end> false" << std::endl;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( false );
	}

	else if( not theTimedCondition.isEqualTrue() )
	{
		apED.makeWritable();

		if( NODE_CONDITION_COMPUTATION_ENABLED
			|| apED.isEnabledNodeCondition() )
		{
			if( not addNodeTimedCondition(apED, theTimedCondition) )
			{
				return( false );
			}
		}

		if( apED.getPathTimedCondition().isEqualTrue() )
		{
			apED.setPathTimedCondition( theTimedCondition );
		}
		else
		{
			apED.setPathTimedCondition( ExpressionSimplifier::simplif_and(
					apED.getPathTimedCondition(), theTimedCondition) );
		}

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "APPEND PATH TIMED CONDITION :end>"
			<< str_indent( apED.getPathTimedCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )
	}

	return( true );
}


bool PathConditionProcessor::appendPathTimedCondition(
		ExecutionData & apED, BF & theTimedCondition,
		CollectionOfExecutionData & listOfOutputED)
{
	if( theTimedCondition.isEqualFalse() ||
			apED.getPathTimedCondition().isEqualFalse() )
	{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << "APPEND PATH TIMED CONDITION : false" << std::endl;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( false );
	}

	else if( theTimedCondition.isEqualTrue() )
	{
		listOfOutputED.append( apED );

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "APPEND PATH TIMED CONDITION :"
			<< str_indent( apED.getPathTimedCondition() ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( true );
	}

	else if( PATH_CONDIDITON_DISJONCTION_SEPARATION_ENABLED &&
			theTimedCondition.is< AvmCode >() &&
			theTimedCondition.to< AvmCode >().isOpCode( AVM_OPCODE_OR ) )
	{
		ExecutionData newED;
		std::size_t count = 0;

		if( apED.getPathTimedCondition().isEqualTrue() )
		{
			for( const auto & itOperand :
				theTimedCondition.to< AvmCode >().getOperands() )
			{
				newED = apED;
				newED.makeWritable();

				if( setTimedCondition(newED, itOperand, itOperand) )
				{
					listOfOutputED.append( newED );
					++count;
				}
			}
		}

		else
		{
			BF thePathTimedCondition;

			for( const auto & itOperand :
				theTimedCondition.to< AvmCode >().getOperands() )
			{
				newED = apED;
				newED.makeWritable();

				thePathTimedCondition = ExpressionSimplifier::simplif_and(
						newED.getPathTimedCondition(), itOperand);
				if( setTimedCondition(newED, itOperand, thePathTimedCondition) )
				{
					listOfOutputED.append( newED );
				}
			}
		}

		return( count > 0 );
	}
	else
	{
		BF thePathTimedCondition;
		if( apED.getPathTimedCondition().isEqualTrue() )
		{
			thePathTimedCondition = theTimedCondition;
		}
		else
		{
			thePathTimedCondition = ExpressionSimplifier::simplif_and(
					apED.getPathTimedCondition(), theTimedCondition);
		}

		if( setTimedCondition(apED, theTimedCondition, thePathTimedCondition) )
		{
			listOfOutputED.append( apED );

			return( true );
		}

		return( false );
	}
}


bool PathConditionProcessor::setTimedCondition(ExecutionData & apED,
		const BF & theNodeTimedCondition, const BF & thePathTimedCondition)
{
	// Verification de la satisfiabilite des gardes
	// compte tenu du parametrage dans favm
	if( PathConditionProcessor::isWeakSatisfiable(
			apED.andPathTimedCondition(thePathTimedCondition)) )
	{
		apED.makeWritable();

		if( NODE_CONDITION_COMPUTATION_ENABLED
			|| apED.isEnabledNodeCondition() )
		{
			if( not addNodeTimedCondition(apED, theNodeTimedCondition) )
			{
				return( false );
			}
		}

		apED.setPathTimedCondition( thePathTimedCondition );

AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << DEFAULT_WRAP_DATA << "SET PATH TIMED CONDITION :"
			<< str_indent( thePathTimedCondition ) << END_WRAP_EOL;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( true );
	}
	else
	{
AVM_IF_DEBUG_FLAG( TEST_DECISION )
	AVM_OS_TRACE << "SET PATH TIMED CONDITION :end> false" << std::endl;
AVM_ENDIF_DEBUG_FLAG( TEST_DECISION )

		return( false );
	}
}


}
