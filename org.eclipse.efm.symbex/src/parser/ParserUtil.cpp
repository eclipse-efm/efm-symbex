/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 9 juil. 2014
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#include "ParserUtil.h"

#include <builder/primitive/CompilationEnvironment.h>

#include <fml/builtin/String.h>

#include <fml/common/LocationElement.h>
#include <fml/common/ObjectElement.h>
#include <fml/common/SpecifierElement.h>
#include <fml/common/TraceableElement.h>

#include <fml/executable/ExecutableLib.h>
#include <fml/executable/InstanceOfMachine.h>

#include <fml/expression/AvmCode.h>
#include <fml/expression/BuiltinArray.h>
#include <fml/expression/ExpressionConstructor.h>
#include <fml/expression/StatementConstructor.h>

#include <fml/lib/AvmOperationFactory.h>
#include <fml/lib/IComPoint.h>

#include <fml/operator/Operator.h>
#include <fml/operator/OperatorManager.h>

#include <fml/type/TypeManager.h>

#include <fml/infrastructure/Buffer.h>
#include <fml/infrastructure/Channel.h>
#include <fml/infrastructure/DataType.h>
#include <fml/infrastructure/InstanceSpecifierPart.h>
#include <fml/infrastructure/Machine.h>
#include <fml/infrastructure/Package.h>
#include <fml/infrastructure/Port.h>
#include <fml/infrastructure/Routine.h>
#include <fml/infrastructure/System.h>
#include <fml/infrastructure/Transition.h>
#include <fml/infrastructure/Variable.h>

#include <fml/infrastructure/BehavioralPart.h>
#include <fml/infrastructure/CompositePart.h>
#include <fml/infrastructure/PropertyPart.h>


namespace sep
{


////////////////////////////////////////////////////////////////////////////////
// XLIA MACRO
////////////////////////////////////////////////////////////////////////////////

#define OP(op)   OperatorManager::OPERATOR_##op

#define NEW_ID(id)   sep::ExpressionConstructor::newIdentifier(id)

#define NEW_UFID(qnid, nb)  ( (nb > 1) ?            \
		ExpressionConstructor::newQualifiedIdentifier(qnid) :      \
		ExpressionConstructor::newIdentifier(qnid) )

#define NEW_INSTANCE_UFID(machine, var)  \
		ExpressionConstructor::newQualifiedIdentifier(  \
				OSS() << machine->getNameID() << '.' << var->getNameID() )


////////////////////////////////////////////////////////////////////////////////
// Current Parse Context :
// System -> Machine -> Routine
////////////////////////////////////////////////////////////////////////////////

ParserUtil::AvmParseContext ParserUtil::_CTX_;

std::stack< ParserUtil::AvmParseContext > ParserUtil::_CTX_STACK_;

void ParserUtil::pushCTX(System * system)
{
	_CTX_.system = system;

	_CTX_.machine = system;

	_CTX_STACK_.push( _CTX_ );
}


////////////////////////////////////////////////////////////////////////////////
// XLIA_SYNTAX_ERROR_COUNT
////////////////////////////////////////////////////////////////////////////////

std::size_t ParserUtil::XLIA_SYNTAX_ERROR_COUNT = 0;


////////////////////////////////////////////////////////////////////////////////
// SET LOCATION IN TRACEABLE FORM
////////////////////////////////////////////////////////////////////////////////

void ParserUtil::setLocation(TraceableElement * aTraceableElement,
		std::size_t bLine, std::size_t eLine)
{
	if( aTraceableElement != nullptr)
	{
		ParserUtil::setLocation( *aTraceableElement, bLine, eLine );
	}
}

void ParserUtil::setLocation(TraceableElement & aTraceableElement,
		std::size_t bLine, std::size_t eLine)
{
	if( not aTraceableElement.hasLocation() )
	{
		aTraceableElement.setLocation( new LocationElement() );
	}
	aTraceableElement.getLocation()->setLine( bLine , eLine );
}


////////////////////////////////////////////////////////////////////////////////
// PROLOGUE OPTION
////////////////////////////////////////////////////////////////////////////////

void ParserUtil::setPrologueOption(const std::string & id, BF value)
{
	if( REGEX_MATCH(id, CONS_WID2("check", "type")) )
	{
		COMPILE_CONTEXT::DEFAUL_TYPE_CHECKING_MASK = value.isNotEqualFalse();
	}

	else if( REGEX_MATCH(id, CONS_WID2("inline", "disable")) )
	{
		COMPILE_CONTEXT::INLINE_DISABLE_MASK = value.isNotEqualFalse();
	}
	else if( REGEX_MATCH(id, CONS_WID2("inline", "enable")) )
	{
		COMPILE_CONTEXT::INLINE_ENABLE_MASK = value.isNotEqualFalse();
	}

	else if( REGEX_MATCH(id, CONS_WID2("inline", "procedure")) )
	{
		COMPILE_CONTEXT::INLINE_PROCEDURE_MASK = value.isNotEqualFalse();
	}

	else if( REGEX_MATCH(id, CONS_WID2("string", "delimiter")) )
	{
		if( value.isCharacter() )
		{
			String::_EMPTY_.to< String >().setQuoteChar(
					String::DEFAULT_DELIMITER = value.toCharacter() );
		}
		else if( value.isBuiltinString() )
		{
			std::string delim = value.toBuiltinString();

			String::_EMPTY_.to< String >().setQuoteChar(
					String::DEFAULT_DELIMITER =
							( delim.empty() ? '\0' : delim[0] ) );
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET VARIABLE BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const BF & ParserUtil::getObjectByNameID(const std::string & aNameID)
{
	if( _CTX_.local != nullptr )
	{
		const BF & var = _CTX_.local->getObjectByNameID(aNameID);
		if( var.valid() )
		{
			return( var );
		}
	}

	if( _CTX_.routine != nullptr )
	{
		const BF & var = _CTX_.routine
				->getPropertyPart().getVariableParameterReturn(aNameID);
		if( var.valid() )
		{
			return( var );
		}
	}

	{
		const BF & form = _CTX_.machine->getsemObjectByNameID(aNameID);
		if( form.valid() )
		{
			return( form );
		}
	}

	return( BF::REF_NULL );
//	return( NEW_ID(aNameID) );
}


const BF & ParserUtil::getObjectByQualifiedNameID(
		const std::string & qnid, std::size_t count)
{
	if( count == 1 )
	{
		return( getObjectByNameID(qnid) );
	}
	else
	{
		const BF & form = _CTX_.machine->getObjectByQualifiedNameID(qnid);
		if( form.valid() )
		{
			return( form );
		}
	}

	return( BF::REF_NULL );
//	return( NEW_UFID(qnid, count) );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET VARIABLE BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const BF & ParserUtil::getvar(const std::string & qnid, std::size_t count)
{
	if( count > 1 )
	{
		const BF & var = _CTX_.machine->getrecVariable(qnid);
		if( var.valid() )
		{
			return( var );
		}
	}
	else
	{
		if( _CTX_.local != nullptr )
		{
			const BF & var = _CTX_.local->getObjectByNameID(qnid);
			if( var.valid() )
			{
				return( var );
			}
		}

		if( _CTX_.routine != nullptr )
		{
			const BF & var = _CTX_.routine
					->getPropertyPart().getVariableParameterReturn(qnid);
			if( var.valid() )
			{
				return( var );
			}
		}

		{
			const BF & var = _CTX_.machine->getsemVariable(qnid);
			if( var.valid() )
			{
				return( var );
			}
		}
	}

	return( BF::REF_NULL );
}

BF ParserUtil::getVariable(const std::string & qnid, std::size_t count)
{
	const BF & var = getvar(qnid, count);
	if( var.valid() )
	{
		return( var );
	}
	else
	{
		AVM_OS_WARN << "Error:> unfound variable< " << qnid
				<< " > in " << str_header( _CTX_.machine ) << " !!!"
				<< std::endl;
		++XLIA_SYNTAX_ERROR_COUNT;

		return( BF( new Variable(_CTX_.machine,
				Modifier::PROPERTY_UNDEFINED_MODIFIER,
				TypeManager::INTEGER, qnid) ) );
	}

	return( var );
}


BF ParserUtil::getDataType(const std::string & qnid, std::size_t count)
{
	const BF & var = ( count > 1 )
			? _CTX_.machine->getrecDataType(qnid)
			: _CTX_.machine->getsemDataType(qnid);

	if( var.valid() )
	{
		return( var );
	}

	AVM_OS_WARN << "Error:> unfound typedef " << qnid
			<< " > in " << str_header( _CTX_.machine ) << " !!!"
			<< std::endl;
	++XLIA_SYNTAX_ERROR_COUNT;

	return( BF( new Variable(_CTX_.machine,
			Modifier::PROPERTY_UNDEFINED_MODIFIER,
			TypeManager::INTEGER, qnid) ) );
}

BF ParserUtil::getDataTypeQueue(const std::string & qnid, std::size_t count)
{
	if( count > 1 )
	{
		const BF & var = _CTX_.machine->getrecDataType(qnid);
		if( var.is< DataType >()
			&& var.to< DataType >().hasTypeQueue() )
		{
			return( var );
		}
	}
	else
	{
		const BF & var = _CTX_.machine->getsemDataType(qnid);
		if( var.is< DataType >()
			&& var.to< DataType >().hasTypeQueue() )
		{
			return( var );
		}
	}

	AVM_OS_WARN << "Error:> unexpected a non-queue variable< " << qnid
			<< " > in " << str_header( _CTX_.machine ) << " !!!"
			<< std::endl;
	++XLIA_SYNTAX_ERROR_COUNT;

	return( BF( DataType::newContainer(
			_CTX_.machine, qnid, TYPE_FIFO_SPECIFIER, 1) ) );
}


////////////////////////////////////////////////////////////////////////////////
// GET DELTA / TIME VARIABLE
BF ParserUtil::getVarTime()
{
	Machine * itMachine = _CTX_.machine;
	for( ; (itMachine != nullptr) ; itMachine = itMachine->getContainerMachine() )
	{
		if( itMachine->getPropertyPart().hasTimeVariable() )
		{
			return( itMachine->getPropertyPart().exprTimeVariable() );
		}
	}

	AVM_OS_WARN << "Error:> unfound timed machine container of "
			<< str_header( _CTX_.machine ) << " !!!"
			<< std::endl;
	++XLIA_SYNTAX_ERROR_COUNT;

	return( BF( new Variable(_CTX_.machine,
			Modifier::PROPERTY_UNDEFINED_MODIFIER,
			TypeManager::INTEGER, "$time") ) );
}

BF ParserUtil::getVarDeltaTime()
{
	Machine * itMachine = _CTX_.machine;
	for( ; (itMachine != nullptr) ; itMachine = itMachine->getContainerMachine() )
	{
		if( itMachine->getPropertyPart().hasDeltaTimeVariable() )
		{
			return( itMachine->getPropertyPart().exprDeltaTimeVariable() );
		}
	}

	AVM_OS_WARN << "Error:> unfound timed machine container of "
			<< str_header( _CTX_.machine ) << " !!!"
			<< std::endl;
	++XLIA_SYNTAX_ERROR_COUNT;

	return( BF( new Variable(_CTX_.machine,
			Modifier::PROPERTY_UNDEFINED_MODIFIER,
			TypeManager::INTEGER, "$delta") ) );
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET CONSTANT BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const BF & ParserUtil::getConstant(
		const std::string & qnid, std::size_t count)
{
	const BF & var = getvar(qnid, count);
	if( var.valid() && var.to< Variable >().getModifier().hasFeatureFinal() )
	{
		return( var );
	}

	return( BF::REF_NULL );
}


std::size_t ParserUtil::getIntegerConstant(
		const std::string & qnid, std::size_t count)
{
	const BF & var = getvar(qnid, count);
	if( var.valid() )
	{
		Variable * pVar = var.to_ptr< Variable >();

		if( pVar->getModifier().hasFeatureFinal() && pVar->hasValue()
			&& pVar->getValue().isWeakInteger() )
		{
			return( pVar->getValue().toInteger() );
		}
	}

	avm_syntax_error( "integer_constant (...)" )
			<< "unfound the integer constant Qualified Name ID< " << qnid
			<< " > from the machine< "
			<< _CTX_.machine->getFullyQualifiedNameID() << " >"
			<< SYNTAX_ERROR_EOL;

	return( 0 );
}


avm_float_t ParserUtil::getFloatConstant(
		const std::string & qnid, std::size_t count)
{
	const BF & var = getvar(qnid, count);
	if( var.valid() )
	{
		Variable * pVar = var.to_ptr< Variable >();

		if( pVar->getModifier().hasFeatureFinal() && pVar->hasValue()
			&& pVar->getValue().isWeakFloat() )
		{
			return( pVar->getValue().toFloat() );
		}
	}

	avm_syntax_error( "float_constant(...)" )
			<< "unfound the float constant Qualified Name ID< " << qnid
			<< " > from the machine< "
			<< _CTX_.machine->getFullyQualifiedNameID() << " >"
			<< SYNTAX_ERROR_EOL;

	return( 0.0 );
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET BUFFER BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

BF ParserUtil::getBuffer(Machine * machine,
		const std::string & qnid, std::size_t count)
{
	const BF & buf = ( count > 1 )
			? machine->getrecBuffer(qnid)
			: machine->getsemBuffer(qnid);

	if( buf.valid() )
	{
		return( buf );
	}

//	AVM_OS_WARN << "Warning:> unfound buffer< " << qnid << " > in machine< "
//			<< str_header( machine ) << " >!!!" << std::endl;
//	++XLIA_SYNTAX_ERROR_COUNT;

	return( NEW_UFID(qnid, count) );
}


BF ParserUtil::getvarBuffer(const std::string & qnid, std::size_t count)
{
	const BF & buffer = ( count > 1 )
			? _CTX_.machine->getrecBuffer(qnid)
			: _CTX_.machine->getsemBuffer(qnid);

	if( buffer.valid() )
	{
		return( buffer );
	}

	BF varBuffer = getvar(qnid, count);
	if( varBuffer.valid() )
	{
		return( varBuffer );
	}
	else
	{
		return( NEW_UFID(qnid, count) );
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET CHANNEL BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

BF ParserUtil::getChannel(const std::string & qnid, std::size_t count)
{
	const BF & chan = ( count > 1 )
			? _CTX_.machine->getrecChannel(qnid)
			: _CTX_.machine->getsemChannel(qnid);

	if( chan.valid() )
	{
		return( chan );
	}

//	AVM_OS_WARN << "Warning:> Unfound channel< " << qnid << " > in machine< "
//			<< str_header( _CTX_.machine ) << " >!!!" << std::endl;
//	++XLIA_SYNTAX_ERROR_COUNT;

	return( NEW_UFID(qnid, count) );
}


void ParserUtil::updateSignalRoutingChannel(Modifier::DIRECTION_KIND ioDirection,
		const BFCode & ioCode, const std::string & qnid, std::size_t count)
{
	const BF & chan = ( count > 1 )
			? _CTX_.machine->getrecChannel(qnid)
			: _CTX_.machine->getsemChannel(qnid);

	if( chan.valid() )
	{
//		if( ioCode->first().is< Port >() )
//		{
//			ioCode->first().to_ptr< Port >()->setRoutingChannel(chan);
//		}

		BF chanSignal = chan.to< Channel >().getSignal(
				ioDirection, ioCode->first());

		if( chanSignal.valid() )
		{
			ioCode->set(0, chanSignal);
		}

		return;
	}

	AVM_OS_WARN << "Warning:> unfound channel< " << qnid
			<< " > in " << str_header( _CTX_.machine ) << " !!!"
			<< std::endl;
	//++XLIA_SYNTAX_ERROR_COUNT;

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET PORT BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const BF & ParserUtil::getComPort(Machine * machine,
		const std::string & qnid, std::size_t count)
{
	if( count > 1 )
	{
		if( machine->getSpecifier().isDesignInstanceStatic()
			&& machine->hasType()
			&& machine->getType().is< Machine >() )
		{
			machine = machine->getType().to_ptr< Machine >();
		}

		//return( machine->getrecPort(qnid) );
		const BF & port = machine->getrecPort(qnid);
		if( port.valid() )
		{
			return( port );
		}
	}
	else
	{
		if( machine->getSpecifier().isDesignInstanceStatic()
			&& machine->hasType()
			&& machine->getType().is< Machine >() )
		{
			machine = machine->getType().to_ptr< Machine >();
		}

		//return( machine->getsemPort(qnid) );
		const BF & port = machine->getsemPort(qnid);
		if( port.valid() )
		{
			return( port );
		}
	}

	AVM_OS_WARN << "Error:> unfound port< " << qnid << " > in machine< "
			<< str_header( machine ) << " >!!!" << std::endl;
	++XLIA_SYNTAX_ERROR_COUNT;

	return( BF::REF_NULL );
}


BF ParserUtil::getvarPort(const std::string & qnid, std::size_t count)
{
	const BF & port = ( count > 1 )
			? _CTX_.machine->getrecPort(qnid)
			: _CTX_.machine->getsemPort(qnid);

	if( port.valid() )
	{
		return( port );
	}
	else
	{
		BF varPort( getvar(qnid, count) );
		if( varPort.valid() )
		{
			return( varPort );
		}
		else
		{
			return( NEW_UFID(qnid, count) );
		}
	}
}

void ParserUtil::appendPortParameter(Port * port,
		const std::string & label, BFVector & labelledParams,
		BFList & positionalParams, const BF & param)
{
	if( label.empty() )
	{
		positionalParams.append( param );
	}
	else if( port != nullptr )
	{
		avm_offset_t offset = port->getParameterOffset(label);
		if( offset != AVM_NO_OFFSET )
		{
			labelledParams[ offset ] = param;
		}
		else
		{
			positionalParams.append( param );

			avm_syntax_error( "ParserUtil::appendPortParameter(...)" )
					<< "Unfound labelled < " << label
					<< " > in port< " << str_header( port ) << " >"
					<< SYNTAX_ERROR_EOL;
		}
	}
	else
	{
		positionalParams.append( param );

		avm_syntax_error( "ParserUtil::appendPortParameter(...)" )
				<< "Unfound labelled < " << label
				<< " > in port< UNKNOWN at parsing time >"
				<< SYNTAX_ERROR_EOL;
	}
}


void ParserUtil::computePortParameter(
		Port * port, const BFCode & ioStatement,
		BFVector & labelledParams, BFList & positionalParams)
{
	if( labelledParams.empty() )
	{
		ioStatement->append( positionalParams );
	}
	else if( port != nullptr )
	{
		TableOfVariable::const_raw_iterator it = port->getParameters().begin();
		TableOfVariable::const_raw_iterator endIt = port->getParameters().end();
		for( avm_offset_t offset = 0 ; it != endIt ; ++it , ++offset )
		{
			if( labelledParams[offset].valid() )
			{
				ioStatement->append( labelledParams[offset] );
			}
			else if( (it)->getModifier().hasNatureParameterBind() )
			{
				if( (it)->hasValue() )
				{
					ioStatement->append( (it)->getValue() );
				}
				else
				{
					avm_syntax_error( "ParserUtil::computePortParameter(...)" )
							<< "Unexpected $bind parameters:" << std::endl
							<< "#" << offset << " " << str_header( (*it) )
							<< std::endl << "wihtout bind lvalue for port:"
							<< std::endl << str_header( port )
							<< SYNTAX_ERROR_EOL;
				}
			}
			else if( (it)->getModifier().hasNatureParameter() )
			{
				if( positionalParams.nonempty() )
				{
					ioStatement->append( positionalParams.pop_first() );
				}
				else if( (it)->hasValue() )
				{
					ioStatement->append( (it)->getValue() );
				}
			}
			else if( positionalParams.nonempty() )
			{
				ioStatement->append( positionalParams.pop_first() );
			}
			else
			{
				avm_syntax_error( "ParserUtil::computePortParameter(...)" )
						<< "Unexpected parameters for port< "
						<< str_header( port ) << " >"
						<< SYNTAX_ERROR_EOL;
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET SIGNAL BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const BF & ParserUtil::getComSignal(Machine * machine,
		const std::string & qnid, std::size_t count)
{
	if( count > 1 )
	{
		if( machine->getSpecifier().isDesignInstanceStatic()
			&& machine->hasType()
			&& machine->getType().is< Machine >() )
		{
			machine = machine->getType().to_ptr< Machine >();
		}

		//return( machine->getrecSignal(qnid) );
		const BF & signal = machine->getrecSignal(qnid);
		if( signal.valid() )
		{
			return( signal );
		}
	}
	else
	{
		if( machine->getSpecifier().isDesignInstanceStatic()
			&& machine->hasType()
			&& machine->getType().is< Machine >() )
		{
			machine = machine->getType().to_ptr< Machine >();
		}

		//return( machine->getsemSignal(qnid) );
		const BF & signal = machine->getsemSignal(qnid);
		if( signal.valid() )
		{
			return( signal );
		}
	}

	AVM_OS_WARN << "Error:> unfound signal< " << qnid << " > in machine< "
			<< str_header( machine ) << " >!!!" << std::endl;
	++XLIA_SYNTAX_ERROR_COUNT;

	return( BF::REF_NULL );
}

BF ParserUtil::getvarSignal(const std::string & qnid, std::size_t count)
{
	const BF & signal = ( count > 1 )
			? _CTX_.machine->getrecSignal(qnid)
			: _CTX_.machine->getsemSignal(qnid);

	if( signal.valid() )
	{
		return( signal );
	}
	else
	{
		BF varSignal( getvar(qnid, count) );
		if( varSignal.valid() )
		{
			return( varSignal );
		}
		else
		{
			return( NEW_UFID(qnid, count) );
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET PORT or SIGNAL BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const BF & ParserUtil::getComPortSignal(Machine * machine,
		const std::string & qnid, std::size_t count)
{
	if( count > 1 )
	{
		if( machine->getSpecifier().isDesignInstanceStatic()
			&& machine->hasType()
			&& machine->getType().is< Machine >() )
		{
			machine = machine->getType().to_ptr< Machine >();
		}

		//return( machine->getrecSignal(qnid) );
		const BF & portSignal = machine->getrecPortSignal(qnid);
		if( portSignal.valid() )
		{
			return( portSignal );
		}
	}
	else
	{
		if( machine->getSpecifier().isDesignInstanceStatic()
			&& machine->hasType()
			&& machine->getType().is< Machine >() )
		{
			machine = machine->getType().to_ptr< Machine >();
		}

		//return( machine->getsemSignal(qnid) );
		const BF & portSignal = machine->getsemPortSignal(qnid);
		if( portSignal.valid() )
		{
			return( portSignal );
		}
	}

	AVM_OS_WARN << "Error:> unfound port/signal< " << qnid << " > in machine< "
			<< str_header( machine ) << " >!!!" << std::endl;
	++XLIA_SYNTAX_ERROR_COUNT;

	return( BF::REF_NULL );
}

BF ParserUtil::getvarPortSignal(
		const std::string & qnid, std::size_t count)
{
	const BF & portSignal = ( count > 1 )
			? _CTX_.machine->getrecPortSignal(qnid)
			: _CTX_.machine->getsemPortSignal(qnid);

	if( portSignal.valid() )
	{
		return( portSignal );
	}
	else
	{
		BF var( getvar(qnid, count) );
		if( var.valid() )
		{
			return( var );
		}
		else
		{
			return( NEW_UFID(qnid, count) );
		}
	}
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET ROUTINE BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Routine * ParserUtil::getRoutine(const std::string & aNameID)
{
	Routine * foundRoutine = _CTX_.machine->rawsemRoutineByNameID(aNameID);

	if( foundRoutine == nullptr )
	{
		AVM_OS_WARN << "Error:> unfound routine< " << aNameID
				<< " > in " << str_header( _CTX_.machine ) << " !!!"
				<< std::endl;
		++XLIA_SYNTAX_ERROR_COUNT;

		return( Routine::newDefine((* _CTX_.machine), aNameID) );
	}

	return( foundRoutine );
}

BF ParserUtil::getvarRoutine(const std::string & aNameID)
{
	Routine * foundRoutine = _CTX_.machine->rawsemRoutineByNameID(aNameID);

	if( foundRoutine == nullptr )
	{
		const Operator * op = AvmOperationFactory::get(aNameID);
		if( op != nullptr )
		{
			return( INCR_BF( const_cast< Operator * >(op) ) );
		}
		else
		{
			AVM_OS_WARN << "Error:> unfound routine< " << aNameID
					<< " > in machine< " << str_header( _CTX_.machine )
					<< " >!!!"
					<< std::endl;
			++XLIA_SYNTAX_ERROR_COUNT;
		}

		return( NEW_ID(aNameID) );
	}

	return( INCR_BF(foundRoutine) );
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET PROCEDURE BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Machine * ParserUtil::getProcedure(const std::string & aNameID)
{
	Machine * foundProcedure = _CTX_.machine->rawsemProcedureByNameID(aNameID);

	if( foundProcedure == nullptr )
	{
		AVM_OS_WARN << "Error:> unfound procedure< " << aNameID
				<< " > in " << str_header( _CTX_.machine ) << " !!!"
				<< std::endl;
		++XLIA_SYNTAX_ERROR_COUNT;

		return( Machine::newProcedure(_CTX_.machine, aNameID,
				Specifier::COMPONENT_PROCEDURE_SPECIFIER) );
	}

	return( foundProcedure );
}

BF ParserUtil::getvarProcedure(const std::string & aNameID)
{
	Machine * foundProcedure = _CTX_.machine->rawsemProcedureByNameID(aNameID);

	if( foundProcedure == nullptr )
	{
		AVM_OS_WARN << "Error:> unfound procedure< " << aNameID
				<< " > in " << str_header( _CTX_.machine ) << " !!!"
				<< std::endl;
		++XLIA_SYNTAX_ERROR_COUNT;

		return( NEW_ID(aNameID) );
	}

	return( INCR_BF(foundProcedure) );
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// GET MACHINE BY [ [ Fully ] Qualified ] Name ID
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Machine * ParserUtil::getMachine(Machine * machine,
		const std::string & qnid, std::size_t count)
{
	Machine * foundMachine = ( count > 1 )
			? machine->getrecMachine(qnid)
			: machine->getsemMachineByNameID(qnid);

	if( foundMachine == nullptr )
	{
		AVM_OS_WARN << "Error:> unfound machine< " << qnid
				<< " > in " << str_header( machine ) << " !!!"
				<< std::endl;
		++XLIA_SYNTAX_ERROR_COUNT;

		return( new Machine(machine, qnid,
				Specifier::COMPONENT_EXECUTABLE_SPECIFIER) );
	}

	return( foundMachine );
}

BF ParserUtil::getvarMachine(const std::string & qnid, std::size_t count)
{
	Machine * foundMachine = nullptr;
	if( count > 1 )
	{
		if( (foundMachine = _CTX_.machine->getsemMachine(qnid)) == nullptr )
		{
			foundMachine = _CTX_.machine->getrecMachine(qnid);
		}
	}
	else
	{
		foundMachine = _CTX_.machine->getsemMachineByNameID(qnid);
	}

	if( foundMachine != nullptr )
	{
		return( INCR_BF(foundMachine) );
	}
	else
	{
		const BF & var = getvar(qnid, count);
		if( var.valid() )
		{
			return( var );
		}
		else
		{
			return( NEW_UFID(qnid, count) );
		}
	}
}

BF ParserUtil::getExecutableMachine(const std::string & qnid, std::size_t count)
{
	Machine * foundMachine = ( count > 1 )
			? _CTX_.machine->getrecExecutableMachine(qnid)
			: _CTX_.machine->getsemExecutableMachine(qnid);

	if( foundMachine == nullptr )
	{
		AVM_OS_WARN << "Error:> unfound typedef machine< " << qnid
				<< " > in machine< " << str_header( _CTX_.machine )
				<< " > !!!" << std::endl;
		++XLIA_SYNTAX_ERROR_COUNT;

		return( BF(new Machine(_CTX_.machine, qnid,
				Specifier::COMPONENT_EXECUTABLE_SPECIFIER)) );
	}

	return( INCR_BF(foundMachine) );
}

BF ParserUtil::getSelfExecutableMachine(const std::string & id)
{
	Machine * foundMachine = _CTX_.machine->getSelfExecutableMachine(id);

	if( foundMachine == nullptr )
	{
		AVM_OS_WARN << "Error:> unfound machine< self." << id
				<< " > in machine< " << str_header( _CTX_.machine )
				<< " > !!!" << std::endl;
		++XLIA_SYNTAX_ERROR_COUNT;

		return( BF(new Machine(_CTX_.machine, id,
				Specifier::COMPONENT_EXECUTABLE_SPECIFIER)) );
	}

	return( INCR_BF(foundMachine) );
}

void ParserUtil::appendInstanceMachineParameter(Machine * machine,
		const std::string & label, BFVector & labelledParams,
		BFList & positionalParams, const BF & param)
{
	if( label.empty() )
	{
		positionalParams.append( param );
	}
	else if( labelledParams.nonempty() )
	{
		machine = machine->getType().as_ptr< Machine >();
		avm_offset_t offset = machine->getVariableParameterOffset(label);
		if( offset != AVM_NO_OFFSET )
		{
			labelledParams[ offset ] = param;
		}
		else
		{
			positionalParams.append( param );

			avm_syntax_error( "ParserUtil::appendInstanceMachineParameter(...)" )
					<< "Unfound labelled parameter < " << label
					<< " > in " << str_header( machine )
					<< SYNTAX_ERROR_EOL;
		}
	}
	else
	{
		avm_syntax_error( "ParserUtil::appendInstanceMachineParameter(...)" )
				<< "Unfound the instance machine model < "
				<< str_header( machine ) << " >"
				<< SYNTAX_ERROR_EOL;
	}
}


void ParserUtil::computeInstanceMachineParameter(Machine * machine,
		BFVector & labelledParams, BFList & positionalParams)
{
	TableOfVariable::raw_iterator it = machine->getType().
			as< Machine >().getVariableParameters().begin();
	TableOfVariable::raw_iterator endIt = machine->getType().
			as< Machine >().getVariableParameters().end();
	for( avm_offset_t offset = 0 ; it != endIt ; ++it , ++offset )
	{
		if( labelledParams[offset].valid() )
		{
				machine->getPropertyPart().saveOwnedVariable(
						new Variable(machine,
								Modifier::PROPERTY_PARAMETER_MODIFIER,
								(it), labelledParams[offset]) );
		}
		else if( (it)->getModifier().hasNatureParameterBind() )
		{
			machine->getPropertyPart().dispatchOwnedElement( (*it) );
			if( not (it)->hasValue() )
			{
				avm_syntax_error(
						"ParserUtil::computeInstanceMachineParameter(...)" )
						<< "Unexpected $bind parameters:" << std::endl
						<< "#" << offset << " " << str_header( (*it) )
						<< std::endl << "wihtout value for machine:"
						<< std::endl << str_header( machine )
						<< SYNTAX_ERROR_EOL;
			}
		}
		else if( (it)->getModifier().hasNatureParameter() )
		{
			if( positionalParams.nonempty() )
			{
				machine->getPropertyPart().saveOwnedVariable(
						new Variable( machine,
								Modifier::PROPERTY_PARAMETER_MODIFIER,
								(it), positionalParams.pop_first()) );
			}
			else //if( (it)->hasValue() )
			{
				machine->getPropertyPart().dispatchOwnedElement( (*it) );
			}
		}
		else if( positionalParams.nonempty() )
		{
			machine->getPropertyPart().saveOwnedVariable(
					new Variable( machine,
							Modifier::PROPERTY_PARAMETER_MODIFIER,
							(it), positionalParams.pop_first()) );
		}
		else
		{
			avm_syntax_error( "ParserUtil::computeInstanceMachineParameter(...)" )
					<< "Unexpected parameters for machine< "
					<< str_header( machine ) << " >"
					<< SYNTAX_ERROR_EOL;
		}
	}
}


void ParserUtil::appendInstanceMachineReturn(Machine * machine,
		const std::string & label, BFVector & labelledReturns,
		BFList & positionalReturns, const BF & param)
{
	if( label.empty() )
	{
		positionalReturns.append( param );
	}
	else if( labelledReturns.nonempty() )
	{
		machine = machine->getType().as_ptr< Machine >();
		avm_offset_t offset = machine->getVariableReturnOffset(label);
		if( offset != AVM_NO_OFFSET )
		{
			labelledReturns[ offset ] = param;
		}
		else
		{
			positionalReturns.append( param );

			avm_syntax_error( "ParserUtil::appendInstanceMachineReturn(...)" )
					<< "Unfound labelled parameter < " << label
					<< " > in " << str_header( machine )
					<< SYNTAX_ERROR_EOL;
		}
	}
	else
	{
		avm_syntax_error( "ParserUtil::appendInstanceMachineReturn(...)" )
				<< "Unfound the instance machine model < "
				<< str_header( machine ) << " >"
				<< SYNTAX_ERROR_EOL;
	}
}


void ParserUtil::computeInstanceMachineReturn(Machine * machine,
		BFVector & labelledReturns, BFList & positionalReturns)
{
	TableOfVariable::raw_iterator it =
			machine->getType().as< Machine >().getVariableReturns().begin();
	TableOfVariable::raw_iterator endIt =
			machine->getType().as< Machine >().getVariableReturns().end();
	for( avm_offset_t offset = 0 ; it != endIt ; ++it , ++offset )
	{
		if( labelledReturns[offset].valid() )
		{
				machine->getPropertyPart().saveOwnedVariable(
						new Variable( machine,
								Modifier::PROPERTY_RETURN_PARAMETER_MODIFIER,
								(it), labelledReturns[offset]) );
		}
		else if( (it)->getModifier().hasModifierReturnBind() )
		{
			machine->getPropertyPart().dispatchOwnedElement( (*it) );
			if( not (it)->hasValue() )
			{
				avm_syntax_error(
						"ParserUtil::computeInstanceMachineReturn(...)" )
						<< "Unexpected #bind parameters < #"
						<< offset << " > wihtout value for machine< "
						<< str_header( machine ) << " >"
						<< SYNTAX_ERROR_EOL;
			}
		}
		else
		{
			if( positionalReturns.nonempty() )
			{
				machine->getPropertyPart().saveOwnedVariable(
						new Variable( machine,
								Modifier::PROPERTY_RETURN_PARAMETER_MODIFIER,
								(it), positionalReturns.pop_first()) );
			}
			else //if( (it)->hasValue() )
			{
				machine->getPropertyPart().dispatchOwnedElement( (*it) );
			}
		}
	}
}


void ParserUtil::appendInstanceDynamicPositionalParameter(
		Machine * anInstance, const BF & rvParameter, std::size_t position)
{
	const Machine * aModel = anInstance->getModelMachine();

	BF lvParameter;

	if( aModel != nullptr )
	{
		if( aModel->hasVariableParameter() )
		{
			const PropertyPart & aPropertyPart = aModel->getPropertyPart();

			if( position < aPropertyPart.getVariableParametersCount()  )
			{
				lvParameter = aPropertyPart.getVariableParameter(position);

				anInstance->getUniqBehaviorPart()->seqOnCreate(
						StatementConstructor::newCode(
								OP(ASSIGN), lvParameter, rvParameter ) );
			}
			else
			{
//				avm_syntax_error( "appendInstanceDynamicPositionalParameter(...)" )
//						<< "Unexpected an #dynamic instance with an"
//								"executable model without enough parameter's"
//						<< SYNTAX_ERROR_EOL;
			}
		}
		else
		{
//			avm_syntax_error( "appendInstanceDynamicPositionalParameter(...)" )
//					<< "Unexpected an #dynamic instance with an"
//							"executable model without parameter's"
//					<< SYNTAX_ERROR_EOL;
		}

	}
	else
	{
//		avm_syntax_error( "appendInstanceDynamicPositionalParameter(...)" )
//				<< "Unexpected an #dynamic instance with an unknown model "
//						"at parsing stage"
//				<< SYNTAX_ERROR_EOL;
	}

	if( lvParameter.invalid() )
	{
		lvParameter = ExpressionConstructor::
				newQualifiedPositionalIdentifier(*anInstance, position);

		anInstance->getUniqBehaviorPart()->seqOnCreate(
				StatementConstructor::newCode(
						OP(ASSIGN), lvParameter, rvParameter) );
	}

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// ROUTINE Parameters / Returns arguments
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ParserUtil::appendRoutineParameters(Routine * routine,
		const std::string & label, BFVector & labelledParams,
		BFList & positionalParams, const BF & param)
{
	if( label.empty() )
	{
		positionalParams.append( param );
	}
	else if( routine != nullptr )
	{
		avm_offset_t offset = routine
				->getPropertyPart().getVariableParameterOffsetByNameID(label);
		if( offset != AVM_NO_OFFSET )
		{
			labelledParams[ offset ] = param;
		}
		else
		{
			positionalParams.append( param );

			avm_syntax_error( "ParserUtil::appendRoutineParameters(...)" )
					<< "Unfound labelled parameter < " << label
					<< " > in routine< " << str_header( routine ) << " >"
					<< SYNTAX_ERROR_EOL;
		}
	}
	else
	{
		positionalParams.append( param );

		avm_syntax_error( "ParserUtil::appendRoutineParameters(...)" )
				<< "Unexpected an unknown routine activity"
				<< SYNTAX_ERROR_EOL;
	}
}


void ParserUtil::appendRoutineReturns(Routine * routine,
		const std::string & label, BFVector & labelledReturns,
		BFList & positionalReturns, const BF & param)
{
	if( label.empty() )
	{
		positionalReturns.append( param );
	}
	else if( routine != nullptr )
	{
		avm_offset_t offset = routine
				->getPropertyPart().getVariableReturnOffsetByNameID(label);
		if( offset != AVM_NO_OFFSET )
		{
			labelledReturns[ offset ] = param;
		}
		else
		{
			positionalReturns.append( param );

			avm_syntax_error( "ParserUtil::appendRoutineReturns(...)" )
					<< "Unfound labelled parameter < " << label
					<< " > in routine< " << str_header( routine ) << " >"
					<< SYNTAX_ERROR_EOL;
		}
	}
	else
	{
		positionalReturns.append( param );

		avm_syntax_error( "ParserUtil::appendRoutineReturns(...)" )
				<< "Unexpected an unknown routine activity"
				<< SYNTAX_ERROR_EOL;
	}
}


void ParserUtil::computeRoutineParamReturn(const BF & ctxMachine,
		Routine * routine, BFCode & activityStatement,
		BFVector & labelledParams, BFList & positionalParams,
		BFVector & labelledReturns, BFList & positionalReturns)
{
	Machine * machine = ctxMachine.is< Machine >() ?
			ctxMachine.to_ptr< Machine >() : nullptr;

	if( (routine != nullptr) && (machine != nullptr) )
	{
		BFCode invokeSequende = StatementConstructor::newCode( OP(SEQUENCE) );

		Machine * machine = ctxMachine.is< Machine >() ?
				ctxMachine.to_ptr< Machine >() : nullptr;

		computeActivityRoutineInput(machine, routine,
				invokeSequende, labelledParams, positionalParams);

		computeActivityRoutineDefaultOutput(machine, routine,
				invokeSequende, labelledReturns, positionalReturns);

		invokeSequende->append( activityStatement );

		computeActivityRoutineOutput(machine, routine,
				invokeSequende, labelledReturns, positionalReturns);

		activityStatement = StatementConstructor::newCode(
				OP(CONTEXT_SWITCHER), ctxMachine, invokeSequende );
	}
	else
	{
		if( labelledParams.empty() )
		{
			activityStatement->append( positionalParams );
		}
		else
		{
			computeActivityRoutineInput(machine, routine,
					activityStatement, labelledParams, positionalParams);
		}

		if( labelledReturns.empty() )
		{
			activityStatement->append( positionalReturns );
		}
		else
		{
			computeActivityRoutineOutput(machine, routine,
					activityStatement, labelledReturns, positionalReturns);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// ACTIVITY ROUTINE Parameters / Returns arguments
////////////////////////////////////////////////////////////////////////////////

Machine * ParserUtil::getActivityMachine(const BF & ctxMachine)
{
	if( ctxMachine.is< Machine >() )
	{
		return ctxMachine.to_ptr< Machine >();
	}

	else if( ctxMachine == ExecutableLib::MACHINE_THIS )
	{
		return( _CTX_.machine );
	}
	else if( ctxMachine == ExecutableLib::MACHINE_SELF )
	{
		return( _CTX_.machine );
	}
	else if( ctxMachine == ExecutableLib::MACHINE_PARENT )
	{
		return( _CTX_.machine->getContainerMachine() );
	}

	else if( ctxMachine == ExecutableLib::MACHINE_COMMUNICATOR )
	{
		return( _CTX_.machine );
	}

	else if( ctxMachine == ExecutableLib::MACHINE_COMPONENT_SELF )
	{
		return( _CTX_.machine );
	}
	else if( ctxMachine == ExecutableLib::MACHINE_COMPONENT_PARENT )
	{
		return( _CTX_.machine );
	}
	else if( ctxMachine == ExecutableLib::MACHINE_COMPONENT_COMMUNICATOR )
	{
		return( _CTX_.machine );
	}

	else if( ctxMachine == ExecutableLib::MACHINE_SYSTEM )
	{
		return( _CTX_.system );
	}

	return nullptr;
}

void ParserUtil::computeActivityRoutineParamReturn(const BF & ctxMachine,
		Routine * routine, BFCode & activityStatement,
		BFVector & labelledParams, BFList & positionalParams,
		BFVector & labelledReturns, BFList & positionalReturns)
{
	Machine * machine = ctxMachine.is< Machine >() ?
			ctxMachine.to_ptr< Machine >() : nullptr;

	if( (routine != nullptr) && (machine != nullptr) )
	{
		BFCode invokeSequende = StatementConstructor::newCode( OP(SEQUENCE) );

		Machine * machine = ctxMachine.is< Machine >() ?
				ctxMachine.to_ptr< Machine >() : nullptr;

		computeActivityRoutineInput(machine, routine,
				invokeSequende, labelledParams, positionalParams);

		computeActivityRoutineDefaultOutput(machine, routine,
				invokeSequende, labelledReturns, positionalReturns);

		invokeSequende->append( activityStatement );

		computeActivityRoutineOutput(machine, routine,
				invokeSequende, labelledReturns, positionalReturns);

		activityStatement = StatementConstructor::newCode(
				OP(CONTEXT_SWITCHER), ctxMachine, invokeSequende );
	}
	else
	{
		if( labelledParams.empty() )
		{
			activityStatement->append( positionalParams );
		}
		else
		{
			computeActivityRoutineInput(machine, routine,
					activityStatement, labelledParams, positionalParams);
		}

		if( labelledReturns.empty() )
		{
			activityStatement->append( positionalReturns );
		}
		else
		{
			computeActivityRoutineOutput(machine, routine,
					activityStatement, labelledReturns, positionalReturns);
		}
	}
}


inline static void appendParam(BFCode & paramCode, const BF & bfParamVar,
		Variable * pParamVar, const BF & bfParamValue)
{
	paramCode->append( StatementConstructor::newCode(
			pParamVar->getModifier().hasNatureReference() ?
					OP(ASSIGN_REF) : OP(ASSIGN),
			pParamVar->getModifier().hasNatureMacro() ?
					pParamVar->getBinding() : bfParamVar,
			bfParamValue ) );
}

void ParserUtil::computeActivityRoutineInput(Machine * ctxMachine,
		Routine * routine, BFCode & invokeSequende,
		BFVector & labelledParams, BFList & positionalParams)
{
	BFCode paramCode( OP(ATOMIC_SEQUENCE) );

	Variable * variable = nullptr;
	BFVector::const_iterator it =
			routine->getPropertyPart().getVariableParameters().begin();
	BFVector::const_iterator endIt =
			routine->getPropertyPart().getVariableParameters().end();
	for( avm_offset_t offset = 0 ; it != endIt ; ++it , ++offset )
	{
		variable = (*it).to_ptr< Variable >();

		if( labelledParams[offset].valid() )
		{
			appendParam(paramCode, (*it), variable, labelledParams[offset]);
		}

		else if( variable->getModifier().hasNatureParameterBind() )
		{
			if( variable->hasValue() )
			{
				appendParam(paramCode, (*it), variable, variable->getValue());
			}
			else
			{
				avm_syntax_error( "ParserUtil::computeActivityRoutineInput(...)" )
						<< "Unexpected $bind parameters:" << std::endl
						<< "#" << offset << " " << str_header( variable )
						<< std::endl << "wihtout value for routine:"
						<< std::endl << str_header( routine )
						<< SYNTAX_ERROR_EOL;
			}
		}

//		else if( variable->getModifier().hasNatureParameter() )
//		{
//			if( positionalParams.nonempty() )
//			{
//				appendParam(paramCode, (*it),
//						variable, positionalParams.pop_first());
//			}
//			else if( variable->hasValue() )
//			{
//				appendParam(paramCode, (*it), variable, variable->getValue());
//			}
//		}

		else if( positionalParams.nonempty() )
		{
			appendParam(paramCode, (*it),
					variable, positionalParams.pop_first());
		}
		else if( variable->hasValue() )
		{
			appendParam(paramCode, (*it), variable, variable->getValue());
		}

		else
		{
			avm_syntax_error( "ParserUtil::computeActivityRoutineInput(...)" )
					<< "Unexpected parameters for routine< "
					<< str_header( routine ) <<  " >"
					<< SYNTAX_ERROR_EOL;
		}
	}

	if( paramCode->hasOperand() )
	{
		invokeSequende->append(
				paramCode->hasOneOperand() ? paramCode->first() : paramCode );
	}
}


inline static void appendReturn(BFCode & returnCode, const BF & bfReturnVar,
		Variable * pReturnVar, const BF & bfReturnDest)
{
	returnCode->append( StatementConstructor::newCode(
			pReturnVar->getModifier().hasNatureReference() ?
					OP(ASSIGN_REF) : OP(ASSIGN),
			bfReturnDest,
			pReturnVar->getModifier().hasNatureMacro() ?
					pReturnVar->getBinding() : bfReturnVar ) );
}


void ParserUtil::computeActivityRoutineDefaultOutput(Machine * ctxMachine,
		Routine * routine, BFCode & invokeSequende,
		BFVector & labelledReturns, BFList & positionalReturns)
{
	BFCode returnCode( OP(ATOMIC_SEQUENCE) );

	Variable * variable = nullptr;
	BFVector::const_iterator it =
			routine->getPropertyPart().getVariableReturns().begin();
	BFVector::const_iterator endIt =
			routine->getPropertyPart().getVariableReturns().end();
	for( avm_offset_t offset = 0 ; it != endIt ; ++it , ++offset )
	{
		variable = (*it).to_ptr< Variable >();

		if( labelledReturns[offset].valid() && variable->hasValue() )
		{
			returnCode->append( StatementConstructor::newCode( OP(ASSIGN),
					(*it), variable->getValue() ) );
		}

		else if( variable->getModifier().hasModifierReturnBind() )
		{
			if( variable->hasValue() )
			{
				appendReturn(returnCode, (*it), variable, variable->getValue());
			}
			else
			{
				avm_syntax_error(
						"ParserUtil::computeActivityRoutineDefaultOutput(...)" )
						<< "Unexpected $bind parameters:" << std::endl
						<< "#" << offset << " " << str_header( variable )
						<< std::endl << "wihtout value for routine:"
						<< std::endl << str_header( routine )
						<< SYNTAX_ERROR_EOL;
			}
		}

//		else if( variable->getModifier().isDirectionReturn() )
//		{
//			if( positionalReturns.nonempty() && variable->hasValue() )
//			{
//				returnCode->append( StatementConstructor::newCode( OP(ASSIGN),
//						(*it), variable->getValue() ) );
//			}
//			else if( variable->hasValue() )
//			{
//			}
//		}

		else if( positionalReturns.nonempty() && variable->hasValue() )
		{
			returnCode->append( StatementConstructor::newCode( OP(ASSIGN),
					(*it), variable->getValue() ) );
		}
		else
		{
		}
	}

	if( returnCode->hasOperand() )
	{
		invokeSequende->append(
			returnCode->hasOneOperand() ? returnCode->first() : returnCode );
	}
}


void ParserUtil::computeActivityRoutineOutput(Machine * ctxMachine,
		Routine * routine, BFCode & invokeSequende,
		BFVector & labelledReturns, BFList & positionalReturns)
{
	BFCode returnCode( OP(ATOMIC_SEQUENCE) );

	Variable * variable = nullptr;
	BFVector::const_iterator it = routine
			->getPropertyPart().getVariableReturns().begin();
	BFVector::const_iterator endIt = routine
			->getPropertyPart().getVariableReturns().end();
	for( avm_offset_t offset = 0 ; it != endIt ; ++it , ++offset )
	{
		variable = (*it).to_ptr< Variable >();

		if( labelledReturns[offset].valid() )
		{
			appendReturn(returnCode, (*it), variable, labelledReturns[offset]);
		}
		else if( variable->getModifier().hasModifierReturnBind() )
		{
			if( variable->hasValue() )
			{
				appendReturn(returnCode, (*it), variable, variable->getValue());
			}
			else
			{
				avm_syntax_error( "ParserUtil::computeActivityRoutineOutput(...)" )
						<< "Unexpected #bind returns < #"
						<< offset << " > wihtout value for routine< "
						<< str_header( routine ) << " >"
						<< SYNTAX_ERROR_EOL;
			}
		}
		else if( variable->getModifier().isDirectionReturn() )
		{
			if( positionalReturns.nonempty() )
			{
				appendReturn(returnCode, (*it),
						variable, positionalReturns.pop_first());
			}
			else if( variable->hasValue() )
			{
				appendReturn(returnCode, (*it),
						variable, variable->getValue());
			}
		}
		else if( positionalReturns.nonempty() )
		{
			appendReturn(returnCode, (*it),
					variable, positionalReturns.pop_first());
		}
		else
		{
			avm_syntax_error( "ParserUtil::computeActivityRoutineOutput(...)" )
					<< "Unexpected returns for routine< "
					<< str_header( routine ) << " >"
					<< SYNTAX_ERROR_EOL;
		}
	}

	if( returnCode->hasOperand() )
	{
		invokeSequende->append(
			returnCode->hasOneOperand() ? returnCode->first() : returnCode );
	}
}


////////////////////////////////////////////////////////////////////////////////
// GET XLIA INVOKABLE BY ID
////////////////////////////////////////////////////////////////////////////////

BF ParserUtil::getInvokable(const BF & target, const std::string & aNameID)
{
	const Operator * op = AvmOperationFactory::get(target, aNameID);
	if( op != nullptr )
	{
		return( INCR_BF( const_cast< Operator * >(op) ) );
	}

	return( ExpressionConstructor::newIdentifier(aNameID) );
}



////////////////////////////////////////////////////////////////////////////////
// DECLARE DEFAULT STATE  #final, #terminal, #return  if need
////////////////////////////////////////////////////////////////////////////////

void ParserUtil::declareDefaultEndingStateIfNeed(
		ListOfMachine & needDefaultStateFinal,
		ListOfMachine & needDefaultStateTerminal,
		ListOfMachine & needDefaultStateReturn)
{
	Specifier aMocSpecifier;

	aMocSpecifier.setStateMocFINAL();
	while( needDefaultStateFinal.nonempty() )
	{
		needDefaultStateFinal.back()->saveOwnedElement(
				Machine::newState(
						needDefaultStateFinal.back(),
						"#final", aMocSpecifier) );

		needDefaultStateFinal.pop_back();
	}

	aMocSpecifier.setPseudostateMocTERMINAL();
	while( needDefaultStateTerminal.nonempty() )
	{
		needDefaultStateTerminal.back()->saveOwnedElement(
				Machine::newState(
						needDefaultStateTerminal.back(),
						"#terminal", aMocSpecifier) );

		needDefaultStateTerminal.pop_back();
	}

	aMocSpecifier.setPseudostateMocRETURN();
	while( needDefaultStateReturn.nonempty() )
	{
		needDefaultStateReturn.back()->saveOwnedElement(
				Machine::newState(
						needDefaultStateReturn.back(),
						"#return", aMocSpecifier) );

		needDefaultStateReturn.pop_back();
	}
}


////////////////////////////////////////////////////////////////////////////////
// INLINE XLIA PROCEDURE CALL IN TRANSITION
////////////////////////////////////////////////////////////////////////////////

void ParserUtil::checkProcedureCompositeMocKind(Machine * aProcedure)
{
	if( aProcedure->getSpecifier().hasModelOfComputation() )
	{
		return;
	}

	bool isStateTransitionFlowFlag = false;

	CompositePart::procedure_iterator itProcedure =
			aProcedure->getCompositePart()->machine_begin();
	CompositePart::procedure_iterator endProcedure =
			aProcedure->getCompositePart()->machine_end();
	for( ; itProcedure != endProcedure ; ++itProcedure )
	{
		if( (itProcedure)->getSpecifier().isPseudostate() )
		{
			if( not isStateTransitionFlowFlag )
			{
				isStateTransitionFlowFlag = true;
			}
		}
		else if( (itProcedure)->getSpecifier().isState()
				|| (itProcedure)->getSpecifier().isComponentStatemachine()
				|| (itProcedure)->getSpecifier().isMocStateTransitionSystem() )
		{
			aProcedure->getwSpecifier().setMocStateTransitionSystem();

			return;
		}
		else if( (itProcedure)->getSpecifier().isDesignInstanceStatic()
				&& (itProcedure)->hasInstanceSpecifier() )
		{
			const BF & model = (itProcedure)->getInstanceSpecifier()->getModel();

			if( model.is< Machine >()
				&& model.to< Machine >()
						.getSpecifier().isMocStateTransitionSystem() )
			{
				aProcedure->getwSpecifier().setMocStateTransitionSystem();

				return;
			}
		}
	}

	if( isStateTransitionFlowFlag )
	{
		aProcedure->getwSpecifier().setMocStateTransitionFlow();
	}

//	aProcedure->getwSpecifier().setMocCompositeStructure();
}


static Transition * insertProcedureStatemachineCall(Transition * transition,
		const std::string & refId, Machine * instanceProcedure,
		const BFCode & paramCode, const BFCode &returnCode)
{
	Machine * procedureStateCall = Machine::newStatemachine(
			transition->getSourceContainer(),
			(OSS() << refId << '#' << instanceProcedure->getNameID()),
			Specifier::EXECUTABLE_PROCEDURE_COMPOSITE_SPECIFIER);
//			instanceProcedure->getType());

	transition->getSourceContainer()->saveOwnedElement( procedureStateCall );

	procedureStateCall->copyLocation( instanceProcedure->getLocation() );

	procedureStateCall->saveOwnedElement(
			sep::incrReferenceCount(instanceProcedure) );
	instanceProcedure->updateContainer( transition->getSourceContainer() );

	// Parameters initialization
	if( paramCode.valid() && paramCode->hasOperand() )
	{
		procedureStateCall->getUniqBehaviorPart()->setOnEnable(
				StatementConstructor::newCode( OP(CONTEXT_SWITCHER),
						INCR_BF(instanceProcedure),
						paramCode->hasOneOperand() ?
								paramCode->first().bfCode() : paramCode) );
	}

	// Returns finalization
	if( returnCode.valid() && returnCode->hasOperand() )
	{
		procedureStateCall->getUniqBehaviorPart()->setOnFinal(
//		procedureStatemachine->getUniqBehaviorPart()->setOnReturn(
				StatementConstructor::newCode( OP(CONTEXT_SWITCHER),
						INCR_BF(instanceProcedure),
						returnCode->hasOneOperand() ?
								returnCode->first().bfCode() : returnCode ) );
	}

	Transition * returnTransition = new Transition((* procedureStateCall),
			(OSS() << refId << '#' << instanceProcedure->getNameID() << "#end"),
			Transition::MOC_FINAL_KIND);

	returnTransition->copyLocation( instanceProcedure->getLocation() );


	returnTransition->setTarget( transition->getTarget() );

	transition->setTarget( INCR_BF(procedureStateCall) );
//	transition->fullyUpdateAllNameID( OSS( ) << transition->getNameID()
//			<< '#' << instanceProcedure->getNameID() );

	procedureStateCall->getUniqBehaviorPart()->
			saveOutgoingTransition( returnTransition );

	return( returnTransition );
}

static Transition * insertProcedureMachineCall(Transition * transition,
		const std::string & refId, Machine * instanceProcedure,
		const BFCode & paramCode, const BFCode &returnCode)
{
	transition->getSourceContainer()->saveOwnedElement(
			sep::incrReferenceCount(instanceProcedure) );
	instanceProcedure->updateContainer( transition->getSourceContainer() );

	instanceProcedure->copyLocation( instanceProcedure->getLocation() );

	if( paramCode.valid() && paramCode->hasOperand() )
	{
		instanceProcedure->getUniqBehaviorPart()->seqOnStart( //seqOnCreate(
				paramCode->hasOneOperand() ?
						paramCode->first().bfCode() : paramCode );
	}

	BFCode callingProcedure;

	if( instanceProcedure->hasParamReturn() )
	{
		instanceProcedure->setAutoStart( false );

		BFCode callingSequende = StatementConstructor::newCode( OP(SEQUENCE) );

		callingProcedure = StatementConstructor::newCode( OP(CONTEXT_SWITCHER),
				INCR_BF(instanceProcedure), callingSequende );

		// Parameters initialization
//		if( paramCode.valid() && paramCode->hasOperand() )
//		{
//			callingSequende->append( paramCode->singleton() ?
//					paramCode->first().bfCode() : paramCode );
//		}

		// Calling procedure
		if( not instanceProcedure->getModelMachine()->
				getSpecifier().isFamilyComponentStatemachine() )
		{
			instanceProcedure->getUniqBehaviorPart()->seqOnStart( OP(RUN) );
		}

		callingSequende->append( StatementConstructor::newCode(
				OP(START), INCR_BF(instanceProcedure) ) );

		// Returns finalization
		if( returnCode.valid() && returnCode->hasOperand() )
		{
			callingSequende->append( returnCode->hasOneOperand() ?
					returnCode->first().bfCode() : returnCode );
		}
	}
	else
	{
		callingProcedure = StatementConstructor::newCode(
				OP(RUN), INCR_BF(instanceProcedure) );
	}

	transition->setStatement( StatementConstructor::xnewCodeFlat(
			OP(SEQUENCE), transition->getStatement(), callingProcedure) );

	return( transition );
}



static bool isOneStepRunningMachine(const Machine & aMachine)
{
	if( not aMachine.getSpecifier().isMocStateTransitionFlow() )
	{
		CompositePart::const_machine_iterator itMachine =
				aMachine.getCompositePart()->machine_begin();
		CompositePart::const_machine_iterator endMachine =
				aMachine.getCompositePart()->machine_end();
		for( ; itMachine != endMachine ; ++itMachine )
		{
			if( (itMachine)->getSpecifier().isDesignInstanceStatic() )
			{
				if( (itMachine)->hasModelMachine()
					&& (not isOneStepRunningMachine(
							(* (itMachine)->getModelMachine()) )) )
				{
					return( false );
				}
			}
			else if( (itMachine)->getSpecifier().isComponentExecutable() )
			{
				if( (itMachine)->hasMachine()
					&& (not isOneStepRunningMachine( (itMachine) )) )
				{
					return( false );
				}
			}
			else if( not (itMachine)->getSpecifier().isPseudostate() )
			{
				return( false );
			}
		}
	}

	return( true );
}


static Transition * insertProcedureCall(Transition * transition,
		const std::string & refId, Machine * instanceProcedure)
{
	instanceProcedure->fullyUpdateAllNameID( OSS( ) << refId
			<< '#' << instanceProcedure->getNameID() );

	const Machine * modelProcedure = instanceProcedure->getModelMachine();

	BFCode paramCode( OP(ATOMIC_SEQUENCE) );
	BFCode returnCode( OP(ATOMIC_SEQUENCE) );

	if( instanceProcedure->hasParamReturn() &&
		instanceProcedure->hasModelMachine() )
	{
		// Parameters initialization
		TableOfVariable::const_raw_iterator itParam =
				instanceProcedure->getVariableParameters().begin();
		TableOfVariable::const_raw_iterator endIt =
				instanceProcedure->getVariableParameters().end();
		for( std::size_t offset = 0 ; itParam != endIt ; ++itParam , ++offset )
		{
			if( (itParam)->hasValue() )
			{
				paramCode->append( StatementConstructor::newCode(
					(itParam)->getModifier().hasNatureReference()
							? OP(ASSIGN_REF) : OP(ASSIGN),
					modelProcedure->getVariableParameter(offset),
					(itParam)->getValue() ) );
			}
		}

		// Returns finalization
		itParam = instanceProcedure->getVariableReturns().begin();
		endIt = instanceProcedure->getVariableReturns().end();
		for( std::size_t offset = 0 ; itParam != endIt ; ++itParam , ++offset )
		{
			if( (itParam)->hasValue() )
			{
//				paramCode->append( StatementConstructor::newCode( OP(ASSIGN_REF),
//						modelProcedure->getVariableReturn(offset),
//						(itParam)->getValue() ) );

				returnCode->append( StatementConstructor::newCode(OP(ASSIGN),
						(itParam)->getValue(),
						modelProcedure->getVariableReturn(offset) ) );
			}
		}
	}

	Transition * returnTransition = nullptr;

	if( isOneStepRunningMachine(* modelProcedure) )
	{
		returnTransition = insertProcedureMachineCall(transition,
				refId, instanceProcedure, paramCode, returnCode);
	}
	else
	{
		returnTransition = insertProcedureStatemachineCall(transition,
				refId, instanceProcedure, paramCode, returnCode);
	}

	return( returnTransition );
}

void ParserUtil::inlineTransitionProcedureCall(
		Transition * transition, const std::string & refId)
{
	Machine * sourceState = (& transition->getSource() );

	BFCode statement = transition->getStatement();

	transition->setStatement( BFCode::REF_NULL );

	if( statement->isOpCode(AVM_OPCODE_INVOKE_METHOD)
			&& statement->first().is< Machine >() )
	{
		transition = insertProcedureCall(transition,
				refId, statement->first().to_ptr< Machine >() );

		if( not transition->hasTarget() )
		{
			transition->setStatement( StatementConstructor::xnewCodeFlat(
					OP(SEQUENCE), transition->getStatement(),
					StatementConstructor::newCode( OP(DISABLE_SELF)),
					StatementConstructor::newCode( OP(ENABLE_SET),
							INCR_BF(sourceState) ) ) );

			transition->setStatement( StatementConstructor::xnewCodeFlat(
					OP(SEQUENCE), transition->getStatement(),
					StatementConstructor::newCode(
							OP(ENABLE_SET), INCR_BF(sourceState) ) ));
		}
	}

	else if( OperatorManager::isSequence(statement.getOperator()) )
	{
		BFCode otherStatement( statement.getOperator() );

		for( const auto & itOperand : statement.getOperands() )
		{
			if( itOperand.is< AvmCode >() )
			{
				const AvmCode & callStatement = itOperand.to< AvmCode >();

				if( callStatement.isOpCode(AVM_OPCODE_INVOKE_METHOD)
					&& callStatement.first().is< Machine >() )
				{
					if( otherStatement->hasOperand() )
					{
						transition->setStatement(
							StatementConstructor::xnewCodeFlat(OP(SEQUENCE),
								transition->getStatement(), otherStatement) );

						otherStatement = StatementConstructor::newCode(
								statement.getOperator() );
					}

					transition = insertProcedureCall(transition,
							refId, callStatement.first().to_ptr< Machine >() );

					continue;
				}
			}

			otherStatement->append( itOperand );
		}

		if( otherStatement->hasOperand() )
		{
			transition->setStatement( StatementConstructor::xnewCodeFlat(
					OP(SEQUENCE), transition->getStatement(), otherStatement ));
		}
		if( not transition->hasTarget() )
		{
			transition->setStatement( StatementConstructor::xnewCodeFlat(
					OP(SEQUENCE), transition->getStatement(),
					StatementConstructor::newCode( OP(DISABLE_SELF)),
					StatementConstructor::newCode(
							OP(ENABLE_SET), INCR_BF(sourceState) ) ) );
		}
	}
}


} /* namespace sep */
