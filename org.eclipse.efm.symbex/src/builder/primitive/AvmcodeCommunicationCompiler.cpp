/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 12 avr. 2011
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#include "AvmcodeCommunicationCompiler.h"

#include <fml/expression/AvmCode.h>
#include <fml/expression/ExpressionTypeChecker.h>

#include <fml/infrastructure/ComProtocol.h>

#include <fml/operator/OperatorManager.h>

#include <sew/Configuration.h>


namespace sep
{


////////////////////////////////////////////////////////////////////////////////
// AVMCODE INPUT STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeInputCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode compiledCode = compileExpressionCode(aCTX, aCode);

	const BF & arg = compiledCode->first();

	if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		//!! NOTHING
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{

	}
	else// if( arg.is< InstanceOfData >() )
	{
		compiledCode->setOperator(OperatorManager::OPERATOR_INPUT_VAR);
	}

	return( compiledCode );
}


BFCode AvmcodeInputCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optimizedCode( aCode->getOperator() );

	AvmInstruction * argsInstruction =
			optimizedCode->newInstruction( aCode->size() );

	AvmCode::const_iterator itOperand = aCode->begin();
	AvmCode::const_iterator endOperand = aCode->end();

	BF arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
	optimizedCode->append( arg );

	if( arg.is< InstanceOfPort >() )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		const InstanceOfPort & aPort = arg.to< InstanceOfPort >();

		if( aPort.hasInputRoutingData() )
		{
			const RoutingData & aRoutingData = aPort.getInputRoutingData();

			switch( aRoutingData.getProtocol() )
			{
				case ComProtocol::PROTOCOL_ENVIRONMENT_KIND:
				{
					optimizedCode->setOperator(
							OperatorManager::OPERATOR_INPUT_ENV);
					break;
				}

				case ComProtocol::PROTOCOL_RDV_KIND:
				case ComProtocol::PROTOCOL_MULTIRDV_KIND:
				{
					optimizedCode->setOperator(
							OperatorManager::OPERATOR_INPUT_RDV);
					break;
				}

				case ComProtocol::PROTOCOL_BUFFER_KIND:
				case ComProtocol::PROTOCOL_BROADCAST_KIND:
				case ComProtocol::PROTOCOL_MULTICAST_KIND:
				case ComProtocol::PROTOCOL_UNICAST_KIND:
				case ComProtocol::PROTOCOL_ANYCAST_KIND:
				{
					if( aRoutingData.getBufferInstance().singleton() )
					{
						optimizedCode->setOperator(
								OperatorManager::OPERATOR_INPUT_BUFFER);
					}
					break;
				}

				case ComProtocol::PROTOCOL_TRANSFERT_KIND:
				case ComProtocol::PROTOCOL_UNDEFINED_KIND:
				default:
				{
					break;
				}
			}
		}

		std::size_t paramCount = aPort.getParameterCount();
		for( std::size_t offset = 1 ;
			(++itOperand != endOperand) && (offset <= paramCount) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype =
					&( aPort.getParameterType(offset-1) );

			setArgcodeLValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		for( std::size_t offset = 1 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeLValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::MESSAGE;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		for( std::size_t offset = 1 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeLValue(aCTX, argsInstruction->at(offset), (*itOperand), false);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeInputCompiler::optimizeStatement :> Unexpected << "
				<< str_header( arg ) << " >> as input first argument !!!"
				<< std::endl;

		return( aCode );
	}
}


////////////////////////////////////////////////////////////////////////////////
// AVMCODE INPUT#FROM STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeInputFromCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	if( aCode->size() >= 2 )
	{
		BFCode compilCode( aCode->getOperator() );

		AvmCode::const_iterator itOperand = aCode->begin();
		compilCode.append(
				compileArgRvalue(aCTX, TypeManager::PORT, *itOperand) );

		compilCode.append(
				compileArgRvalue(aCTX, TypeManager::MACHINE, *(++itOperand)) );

		AvmCode::const_iterator endOperand = aCode->end();
		for( ++itOperand ; itOperand != endOperand ; ++itOperand )
		{
			compilCode.append( compileArgLvalue(aCTX, *itOperand) );
		}

		return( compilCode );

	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeInputFromCompiler::compileStatement :> "
					"Not enough parameter for << input_from >> statement << "
				<< str_indent( aCode ) << " >> !!!"
				<< std::endl;

		return( aCode );
	}
}


BFCode AvmcodeInputFromCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optimizedCode( aCode->getOperator() );

	AvmInstruction * argsInstruction =
			optimizedCode->newInstruction( aCode->size() );

	AvmCode::const_iterator itOperand = aCode->begin();
	AvmCode::const_iterator endOperand = aCode->end();

	BF arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
	optimizedCode->append( arg );

	if( arg.is< InstanceOfPort >() )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		const InstanceOfPort & aPort = arg.to< InstanceOfPort >();
		std::size_t paramCount = aPort.getParameterCount();

		arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*(++itOperand)));
		optimizedCode->append( arg );

		argsInstruction->at(1).dtype = TypeManager::MACHINE;
		setArgcodeRValue(aCTX, argsInstruction->at(1), arg);

		for( std::size_t offset = 2 ;
				(++itOperand != endOperand) && (paramCount > 0) ; ++offset , --paramCount )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype =
					&( aPort.getParameterType(offset-2) );

			setArgcodeLValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*(++itOperand)));
		optimizedCode->append( arg );

		argsInstruction->at(1).dtype = TypeManager::MACHINE;
		setArgcodeRValue(aCTX, argsInstruction->at(1), arg);

		for( std::size_t offset = 2 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeLValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::MESSAGE;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);


		arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*(++itOperand)));
		optimizedCode->append( arg );

		argsInstruction->at(1).dtype = TypeManager::MACHINE;
		setArgcodeRValue(aCTX, argsInstruction->at(1), arg);

		for( std::size_t offset = 2 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeLValue(aCTX, argsInstruction->at(offset), (*itOperand), false);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeInputCompiler::optimizeStatement :> Unexpected << "
				<< str_header( arg ) << " >> as input first argument !!!"
				<< std::endl;

		return( aCode );
	}
}



////////////////////////////////////////////////////////////////////////////////
// AVMCODE INPUT#SAVE STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeInputSaveCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{

	return( StatementConstructor::newCode(aCode->getOperator(),
			compileArgRvalue(aCTX, TypeManager::PORT, aCode->first())) );
	return( compileExpressionCode(aCTX, aCode) );
}


BFCode AvmcodeInputSaveCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optimizedCode( aCode->getOperator() );

	AvmInstruction * argsInstruction =
			optimizedCode->newInstruction( aCode->size() );

	BF arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, aCode->first());
	optimizedCode->append( arg );

	if( arg.is< InstanceOfPort >() )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

//		const InstanceOfPort & aPort = arg.to< InstanceOfPort >();

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);


		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::MESSAGE;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeInputCompiler::optimizeStatement :> Unexpected << "
				<< str_header( arg ) << " >> as input first argument !!!"
				<< std::endl;

		return( aCode );
	}
}


////////////////////////////////////////////////////////////////////////////////
// AVMCODE INPUT#VAR STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeInputVarCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BF lvalue = compileArgLvalue(aCTX, aCode->first());

	if( aCode->hasManyOperands() )
	{
		if( lvalue.is< InstanceOfData >() )
		{
			aCTX = aCTX->clone(
					lvalue.to< InstanceOfData >().getTypeSpecifier() );
		}
		return( StatementConstructor::newCode( aCode->getOperator(),
				lvalue, compileArgRvalue(aCTX, aCode->second())) );
	}
	else
	{
		return( StatementConstructor::newCode(aCode->getOperator(), lvalue) );
	}
}


BFCode AvmcodeInputVarCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	AvmInstruction * argsInstruction = aCode->genInstruction();

	setArgcodeLValue(aCTX, argsInstruction->at(0), aCode->first());

	if( aCode->hasManyOperands() )
	{
		optimizeArgExpression(aCTX, aCode, 1);
		argsInstruction->at(1).dtype = argsInstruction->at(0).dtype;
		setArgcodeRValue(aCTX, argsInstruction->at(1), aCode->second());
	}

	argsInstruction->computeMainBytecode(
			/*context  */ AVM_ARG_STANDARD_CTX,
			/*processor*/ AVM_ARG_STATEMENT_CPU,
			/*operation*/ AVM_ARG_SEVAL_RVALUE,
			/*operand  */ AVM_ARG_STATEMENT_KIND,
			/*dtype    */ argsInstruction->at(0).dtype);

	return( aCode );
}


////////////////////////////////////////////////////////////////////////////////
// AVMCODE INPUT#ENV STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeInputEnvCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	return( compileExpressionCode(aCTX, aCode) );
}


BFCode AvmcodeInputEnvCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optimizedCode( aCode->getOperator() );

	AvmInstruction * argsInstruction =
			optimizedCode->newInstruction( aCode->size() );

	AvmCode::const_iterator itOperand = aCode->begin();
	AvmCode::const_iterator endOperand = aCode->end();

	BF arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
	optimizedCode->append( arg );

	if( arg.is< InstanceOfPort >() )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		const InstanceOfPort & aPort = arg.to< InstanceOfPort >();
		std::size_t paramCount = aPort.getParameterCount();

		for( std::size_t offset = 1 ;
			(++itOperand != endOperand) && (paramCount > 0) ; ++offset , --paramCount )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype =
					&( aPort.getParameterType(offset-1) );

			setArgcodeLValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		for( std::size_t offset = 1 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeLValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::MESSAGE;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		for( std::size_t offset = 1 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeLValue(aCTX, argsInstruction->at(offset), (*itOperand), false);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeInputCompiler::optimizeStatement :> Unexpected << "
				<< str_header( arg ) << " >> as input first argument !!!"
				<< std::endl;

		return( aCode );
	}
}



////////////////////////////////////////////////////////////////////////////////
// AVMCODE OUTPUT STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeOutputCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode compiledCode = compileExpressionCode(aCTX, aCode);

	const BF & arg = compiledCode->first();

	if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		//!! NOTHING
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{

	}
	else
	{
		compiledCode->setOperator(OperatorManager::OPERATOR_OUTPUT_VAR);
	}

	return( compiledCode );
}


BFCode AvmcodeOutputCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optimizedCode( aCode->getOperator() );

	AvmInstruction * argsInstruction =
			optimizedCode->newInstruction( aCode->size() );

	AvmCode::iterator itOperand = aCode->begin();
	AvmCode::iterator endOperand = aCode->end();

	BF arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
	optimizedCode->append( arg );

	if( arg.is< InstanceOfPort >() )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		const InstanceOfPort & aPort = arg.to< InstanceOfPort >();

		if( aPort.hasOutputRoutingData() )
		{
			const RoutingData & aRoutingData = aPort.getOutputRoutingData();

			switch( aRoutingData.getProtocol() )
			{
				case ComProtocol::PROTOCOL_ENVIRONMENT_KIND:
				{
					optimizedCode->setOperator(
							OperatorManager::OPERATOR_OUTPUT_ENV);
					break;
				}

				case ComProtocol::PROTOCOL_RDV_KIND:
				case ComProtocol::PROTOCOL_MULTIRDV_KIND:
				{
					optimizedCode->setOperator(
							OperatorManager::OPERATOR_OUTPUT_RDV);
					break;
				}

				case ComProtocol::PROTOCOL_BUFFER_KIND:
				case ComProtocol::PROTOCOL_BROADCAST_KIND:
				case ComProtocol::PROTOCOL_MULTICAST_KIND:
				case ComProtocol::PROTOCOL_UNICAST_KIND:
				case ComProtocol::PROTOCOL_ANYCAST_KIND:
				{
					if( aRoutingData.getBufferInstance().singleton() )
					{
						optimizedCode->setOperator(
								OperatorManager::OPERATOR_OUTPUT_BUFFER);
					}
					break;
				}

				case ComProtocol::PROTOCOL_TRANSFERT_KIND:
				case ComProtocol::PROTOCOL_UNDEFINED_KIND:
				default:
				{
					break;
				}
			}
		}

		std::size_t paramCount = aPort.getParameterCount();
		for( std::size_t offset = 1 ;
			(++itOperand != endOperand) && (offset <= paramCount) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype =
					&( aPort.getParameterType(offset-1) );

			setArgcodeRValue(
					aCTX->clone( aPort.getParameterType(offset-1) ),
					argsInstruction->at(offset), arg );
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		for( std::size_t offset = 1 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeRValue(aCTX, argsInstruction->at(offset), arg, false);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::MESSAGE;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		for( std::size_t offset = 1 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeRValue(aCTX,
					argsInstruction->at(offset), (*itOperand), false);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeInputCompiler::optimizeStatement :> Unexpected << "
				<< str_header( arg ) << " >> as input first argument !!!"
				<< std::endl;

		return( aCode );
	}
}


////////////////////////////////////////////////////////////////////////////////
// AVMCODE OUTPUT#TO STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeOutputToCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	if( aCode->size() >= 2 )
	{
		BFCode compilCode( aCode->getOperator() );

		AvmCode::const_iterator itOperand = aCode->begin();
		compilCode.append(
				compileArgRvalue(aCTX, TypeManager::PORT, *itOperand) );

		compilCode.append(
				compileArgRvalue(aCTX, TypeManager::MACHINE, *(++itOperand)) );

		AvmCode::const_iterator endOperand = aCode->end();
		for( ++itOperand ; itOperand != endOperand ; ++itOperand )
		{
			compilCode.append( compileArgRvalue(aCTX, *itOperand) );
		}

		return( compilCode );

	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeOutputToCompiler::compileStatement :> "
					"Not enough parameter for << output_to >> statement << "
				<< str_indent( aCode ) << " >> !!!"
				<< std::endl;

		return( aCode );
	}
}


BFCode AvmcodeOutputToCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optimizedCode( aCode->getOperator() );

	AvmInstruction * argsInstruction =
			optimizedCode->newInstruction( aCode->size() );

	AvmCode::iterator itOperand = aCode->begin();
	AvmCode::iterator endOperand = aCode->end();

	BF arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
	optimizedCode->append( arg );

	if( arg.is< InstanceOfPort >() )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		const InstanceOfPort & aPort = arg.as< InstanceOfPort >();
		std::size_t paramCount = aPort.getParameterCount();


		arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*(++itOperand)));
		optimizedCode->append( arg );

		argsInstruction->at(1).dtype = TypeManager::MACHINE;
		setArgcodeRValue(aCTX, argsInstruction->at(1), arg);

		for( std::size_t offset = 2 ;
			(++itOperand != endOperand) && (paramCount > 0) ;
			++offset , --paramCount )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype =
					&( aPort.getParameterType(offset-2) );
			setArgcodeRValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*(++itOperand)));
		optimizedCode->append( arg );

		argsInstruction->at(1).dtype = TypeManager::MACHINE;
		setArgcodeRValue(aCTX, argsInstruction->at(1), arg);

		for( std::size_t offset = 2 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeRValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::MESSAGE;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);


		arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*(++itOperand)));
		optimizedCode->append( arg );

		argsInstruction->at(1).dtype = TypeManager::MACHINE;
		setArgcodeRValue(aCTX, argsInstruction->at(1), arg);

		for( std::size_t offset = 2 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeRValue(aCTX,
					argsInstruction->at(offset), (*itOperand), false);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeInputCompiler::optimizeStatement :> Unexpected << "
				<< str_header( arg ) << " >> as input first argument !!!"
				<< std::endl;

		return( aCode );
	}
}



////////////////////////////////////////////////////////////////////////////////
// AVMCODE OUTPUT#VAR STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeOutputVarCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BF lvalue = compileArgLvalue(aCTX, aCode->first());

	if( aCode->hasManyOperands() )
	{
		if( lvalue.is< InstanceOfData >() )
		{
			aCTX = aCTX->clone( lvalue.to<
					InstanceOfData >().getTypeSpecifier() );
		}
		return( StatementConstructor::newCode( aCode->getOperator(),
				lvalue, compileArgRvalue(aCTX, aCode->second())) );
	}
	else
	{
		return( StatementConstructor::newCode(
				aCode->getOperator(), lvalue, lvalue) );
	}
}


BFCode AvmcodeOutputVarCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	if( aCode->hasOneOperand() )
	{
		aCode->append( aCode->first() );
	}

	AvmInstruction * argsInstruction = aCode->genInstruction();

	setArgcodeLValue(aCTX, argsInstruction->at(0), aCode->first());

	if( aCode->hasOneOperand() )
	{
		aCode->append( aCode->first() );
	}

	optimizeArgExpression(aCTX, aCode, 1);
	argsInstruction->at(1).dtype = argsInstruction->at(0).dtype;
	setArgcodeRValue(aCTX, argsInstruction->at(1), aCode->second());

	argsInstruction->computeMainBytecode(
			/*context  */ AVM_ARG_STANDARD_CTX,
			/*processor*/ AVM_ARG_STATEMENT_CPU,
			/*operation*/ AVM_ARG_SEVAL_RVALUE,
			/*operand  */ AVM_ARG_STATEMENT_KIND,
			/*dtype    */ argsInstruction->at(0).dtype);

	return( aCode );
}



////////////////////////////////////////////////////////////////////////////////
// AVMCODE OUTPUT#ENV STATEMENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BFCode AvmcodeOutputEnvCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	return( compileExpressionCode(aCTX, aCode) );
}


BFCode AvmcodeOutputEnvCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optimizedCode( aCode->getOperator() );

	AvmInstruction * argsInstruction =
			optimizedCode->newInstruction( aCode->size() );

	AvmCode::iterator itOperand = aCode->begin();
	AvmCode::iterator endOperand = aCode->end();

	BF arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
	optimizedCode->append( arg );

	if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		const InstanceOfPort & aPort = arg.as< InstanceOfPort >();
		std::size_t paramCount = aPort.getParameterCount();

		for( std::size_t offset = 1 ;
			(++itOperand != endOperand) && (paramCount > 0) ;
			++offset , --paramCount )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype =
					&( aPort.getParameterType(offset-1) );
			setArgcodeRValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::PORT, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::PORT;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		for( std::size_t offset = 1 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeRValue(aCTX, argsInstruction->at(offset), arg);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else if( ExpressionTypeChecker::isTyped(TypeManager::MESSAGE, arg) )
	{
		argsInstruction->at(0).dtype = TypeManager::MESSAGE;
		setArgcodeRValue(aCTX, argsInstruction->at(0), arg, false);

		for( std::size_t offset = 1 ; (++itOperand != endOperand) ; ++offset )
		{
			arg = AVMCODE_COMPILER.decode_optimizeExpression(aCTX, (*itOperand));
			optimizedCode->append( arg );

			argsInstruction->at(offset).dtype = TypeManager::UNIVERSAL;
			setArgcodeRValue(aCTX,
					argsInstruction->at(offset), (*itOperand), false);
		}

		argsInstruction->computeMainBytecode(
				/*context  */ AVM_ARG_STANDARD_CTX,
				/*processor*/ AVM_ARG_STATEMENT_CPU,
				/*operation*/ AVM_ARG_SEVAL_RVALUE,
				/*operand  */ AVM_ARG_STATEMENT_KIND);

		return( optimizedCode );
	}
	else
	{
		getCompilerTable().incrErrorCount();
		aCTX->errorContext( AVM_OS_WARN )
				<< "AvmcodeInputCompiler::optimizeStatement :> Unexpected << "
				<< str_header( arg ) << " >> as input first argument !!!"
				<< std::endl;

		return( aCode );
	}
}



////////////////////////////////////////////////////////////////////////////////
// AVMCODE PRESENT COMPILATION
////////////////////////////////////////////////////////////////////////////////

BF AvmcodeAbsentPresentCompiler::compileExpression(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	return( compileExpressionCode(aCTX, aCode) );
}

BF AvmcodeAbsentPresentCompiler::optimizeExpression(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optCode = optimizeExpressionCode(aCTX, aCode, AVM_ARG_EXPRESSION_KIND) ;

	optCode->getInstruction()->setMainBytecode(
			/*context  */ AVM_ARG_RETURN_CTX,
			/*processor*/ AVM_ARG_ARITHMETIC_LOGIC_CPU,
			/*operation*/ AVM_ARG_SEVAL_RVALUE,
			/*operand  */ AVM_ARG_EXPRESSION_KIND,
			/*dtype    */ TypeManager::BOOLEAN.rawType() );


	return( optCode );
}


BFCode AvmcodeAbsentPresentCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	return( compileExpressionCode(aCTX, aCode) );
}

BFCode AvmcodeAbsentPresentCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	return( optimizeStatementCode(aCTX, aCode) );
}



////////////////////////////////////////////////////////////////////////////////
// AVMCODE OBS COMPILATION
////////////////////////////////////////////////////////////////////////////////

inline BF AvmcodeObsCompiler::compileExpression(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	return( compileStatement(aCTX, aCode) );
}

inline BF AvmcodeObsCompiler::optimizeExpression(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	BFCode optCode = optimizeStatement(aCTX, aCode);

	optCode->getInstruction()->setMainOperand( AVM_ARG_EXPRESSION_KIND );

	return( optCode );
}


inline BFCode AvmcodeObsCompiler::compileStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	if( aCode->size() == 2 )
	{
		return( StatementConstructor::newCode( aCode->getOperator(),
				AVMCODE_COMPILER.getConfiguration().
					getExecutableSystem().getSystemInstance(),
				compileArgStatement(aCTX, aCode->first()),
				compileArgRvalue(aCTX, TypeManager::BOOLEAN, aCode->second())) );
	}
	return( StatementConstructor::newCode( aCode->getOperator(),
			compileArgRvalue(aCTX, TypeManager::MACHINE, aCode->first()),
			compileArgStatement(aCTX, aCode->second()),
			compileArgRvalue(aCTX, TypeManager::BOOLEAN, aCode->operand(2))) );
}

// lvalue =: rvalue;  ==>  [ lvalue , rvalue ]
BFCode AvmcodeObsCompiler::optimizeStatement(
		COMPILE_CONTEXT * aCTX, const BFCode & aCode)
{
	AvmInstruction * argsInstruction = aCode->genInstruction();

	optimizeArgStatement(aCTX, aCode, 0);
	setArgcodeRValue(aCTX, argsInstruction->at(0), aCode->first());
	argsInstruction->operand(0, AVM_ARG_MACHINE_RID);

	optimizeArgStatement(aCTX, aCode, 1);
	setArgcodeRValue(aCTX, argsInstruction->at(1), aCode->second());
	AvmCode & secondCode = aCode->second().to< AvmCode >();
	if( secondCode.getInstruction()->populated() )
	{
		if( secondCode[1].is< InstanceOfData >() )
		{
			secondCode.getInstruction()->at(1).setNopsCpu();
		}
		else
		{
			getCompilerTable().incrErrorCount();
			aCTX->errorContext( AVM_OS_WARN )
					<< "AvmcodeObsCompiler::optimizeStatement :> Unexpected << "
					<< secondCode[1].str()
					<< " >>, which is not a Variable, as argument of "
					<< secondCode.strOperator()
					<< " in @observe{..}[..] statement!!!"
					<< std::endl;

			return( aCode );
		}

		for( std::size_t offset = 2 ;
				offset < secondCode.getInstruction()->size() ; ++offset )
		{
			if( secondCode[offset].is< InstanceOfData >() )
			{
				secondCode.getInstruction()->at(offset).setNopCpu();
			}
			else
			{
				getCompilerTable().incrErrorCount();
				aCTX->errorContext( AVM_OS_WARN )
						<< "AvmcodeObsCompiler::optimizeStatement :> Unexpected << "
						<< secondCode[offset].str()
						<< " >>, which is not a Variable, as argument of "
						<< secondCode.strOperator()
						<< " in @observe{..}[..] statement!!!"
						<< std::endl;

				return( aCode );
			}
		}
	}

	optimizeArgExpression(aCTX, aCode, 2);
	argsInstruction->at(2).dtype = TypeManager::BOOLEAN;
	setArgcodeRValue(aCTX, argsInstruction->at(2), aCode->operand(2));

	argsInstruction->computeMainBytecode(
			/*context  */ AVM_ARG_STANDARD_CTX,
			/*processor*/ AVM_ARG_STATEMENT_CPU,
			/*operation*/ AVM_ARG_SEVAL_RVALUE,
			/*operand  */ AVM_ARG_STATEMENT_KIND,
			/*dtype    */ argsInstruction->at(0).dtype);

	return( aCode );
}



}
