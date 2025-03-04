/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 7 janv. 2014
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#include "AvmTestCaseFactory.h"
#include "AvmTestCaseStatistics.h"
#include "AvmTestCaseUtils.h"

#include <collection/BFContainer.h>

#include <computer/BaseEnvironment.h>
#include <computer/EvaluationEnvironment.h>

#include <famcore/api/AbstractProcessorUnit.h>
#include <famsc/tcgenerator/AvmPathGuidedTestcaseGenerator.h>

#include <fml/builtin/Identifier.h>

#include <fml/common/ModifierElement.h>

#include <fml/expression/AvmCode.h>
#include <fml/expression/ExpressionConstant.h>
#include <fml/expression/ExpressionFactory.h>
#include <fml/expression/StatementConstructor.h>
#include <fml/expression/StatementFactory.h>

#include <fml/infrastructure/BehavioralPart.h>
#include <fml/infrastructure/CompositePart.h>
#include <fml/infrastructure/ComProtocol.h>
#include <fml/infrastructure/ComRoute.h>
#include <fml/infrastructure/Connector.h>
#include <fml/infrastructure/InteractionPart.h>
#include <fml/infrastructure/Machine.h>
#include <fml/infrastructure/Port.h>
#include <fml/infrastructure/PropertyPart.h>
#include <fml/infrastructure/Transition.h>
#include <fml/infrastructure/System.h>
#include <fml/infrastructure/Variable.h>

#include <fml/runtime/ExecutionConfiguration.h>
#include <fml/runtime/ExecutionContext.h>

#include <fml/template/TimedMachine.h>

#include <fml/type/TypeManager.h>

#include <sew/Configuration.h>

#include <solver/api/SolverFactory.h>
#include <solver/Z3Solver.h>

#include <util/ExecutionChrono.h>


namespace sep
{


static const std::string VAR_TC_CLOCK_ID = "clt"; // a.k.a. "tc_clock"
static const std::string VAR_TM_ID       = "TM";

/**
 * CONSTRUCTOR
 * Default
 */
AvmTestCaseFactory::AvmTestCaseFactory(
		AvmPathGuidedTestcaseGenerator & aProcessor,
		AvmTestCaseStatistics & aTestCaseStatistics,
		const Symbol & aQuiescencePortTP)
: mProcessor(aProcessor),
ENV( aProcessor.getENV() ),
mQuiescencePortTP( aQuiescencePortTP ),
mUncontrollableTraceFilter( mProcessor.getUncontrollableTraceFilter() ),
mTestCaseStatistics( aTestCaseStatistics ),
mTestPurposeTrace( ),
mTestPurposeInoutParams( ),
mTestPurposeClockParams( ),
mVarTC_subst_mParamTP_ED( ),
mTestPurposePathCondition( ),
mSystemSUT( mProcessor.getConfiguration().getSpecification() ),
mSystemTC( "tcSystem", mSystemSUT.getSpecifier() ),
mMachineTC( nullptr ),
mOutputPortSUT_toInputTC( ),
mUncontrollableSUT_toInputTC( ),
mQuiescencePortTC( ),
mVariable_TC_TM( ),
mVariable_TC_Clock( ),
mNewfreshInitialVars( ),
mNewfreshTraceVarsTP( ),
mNewfreshInitialTraceVarsTP( )
//mStateTC_FAIL( nullptr ),
//mStateTC_INC( nullptr )
{
	//!! NOTHING
}


////////////////////////////////////////////////////////////////////////////////
// QUIESCENCE
////////////////////////////////////////////////////////////////////////////////

void AvmTestCaseFactory::buildTestCase()
{
AVM_IF_DEBUG_LEVEL_FLAG( LOW , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "BUILD TEST CASE STRUCTURE " ) << std::flush;
	AVM_OS_DEBUG << INCR_INDENT;
AVM_ENDIF_DEBUG_LEVEL_FLAG( LOW , PROCESSING )

	buildStructure(mSystemSUT, mSystemTC);

	OutStream & out2File = mProcessor.getStream( "file#tc" );
	mSystemTC.toStream(out2File);

	saveTestCaseJson(mSystemTC);

AVM_IF_DEBUG_LEVEL_FLAG( LOW , PROCESSING )
	AVM_OS_DEBUG << DECR_INDENT;
AVM_ENDIF_DEBUG_LEVEL_FLAG( LOW , PROCESSING )
}

void AvmTestCaseFactory::buildStructure(const System & sutSystem, System & tcSystem)
{
	// Main test case machine
	mMachineTC = Machine::newStatemachine(& tcSystem, "tcMachine",
			Specifier::DESIGN_PROTOTYPE_STATIC_SPECIFIER);
	tcSystem.saveOwnedElement(mMachineTC);


//	mStateTC_FAIL = Machine::newState(tcMachine, "FAIL", Specifier::STATE_FINAL_MOC);
//	tcMachine->saveOwnedElement(mStateTC_FAIL);
//
//	mStateTC_INC = Machine::newState(tcMachine, "INC", Specifier::STATE_FINAL_MOC);
//	tcMachine->saveOwnedElement(mStateTC_INC);


	PropertyPart & tcPropertyDecl = mMachineTC->getPropertyPart();
	InteractionPart * tcInteractionPart = mMachineTC->getUniqInteraction();

	// Quiescence port
	Modifier inputModifier;
	inputModifier.setVisibilityPublic().setDirectionInput();

	Port * quiescencePort = new Port(tcPropertyDecl, "Quiescence",
					IComPoint::IO_PORT_NATURE, inputModifier);
	mQuiescencePortTC = tcPropertyDecl.saveOwnedElement(quiescencePort);

	Connector & aConnector = tcInteractionPart->appendConnector(
			ComProtocol::PROTOCOL_ENVIRONMENT_KIND);
	aConnector.appendComRoute(quiescencePort, Modifier::PROPERTY_INPUT_DIRECTION );

	const PropertyPart & sutPropertyDecl = sutSystem.getPropertyPart();

	addPorts(tcPropertyDecl, tcInteractionPart, sutPropertyDecl);

	if( sutSystem.hasMachine() )
	{
		const CompositePart * sutCompositePart = sutSystem.getCompositePart();
		TableOfMachine::const_ref_iterator itm = sutCompositePart->getMachines().begin();
		TableOfMachine::const_ref_iterator endItm = sutCompositePart->getMachines().end();
		for( ; itm != endItm ; ++itm )
		{
			if( itm->hasPortSignal() )
			{
				addPorts(tcPropertyDecl, tcInteractionPart, itm->getPropertyPart());
			}
		}
	}

	ExecutionContext & rootEC =
			mProcessor.getConfiguration().getFirstExecutionTrace();

	AvmTestCaseUtils::getTestPurposeTrace(rootEC, mTestPurposeTrace);

	const ExecutionContext * tpTargetEC = mTestPurposeTrace.last();

	mVarTC_subst_mParamTP_ED = tpTargetEC->getExecutionData();

	mTestPurposePathCondition = tpTargetEC->getPathCondition();

//	AvmTestCaseUtils::getParameters(*tpTargetEC, mTestPurposeParams);
	AvmTestCaseUtils::newfreshTraceParamsFromEC(
			*tpTargetEC, mTestPurposeInoutParams, mTestPurposeClockParams);
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	mTestPurposeInoutParams.strFQN( AVM_OS_DEBUG << "mTestPurposeInoutParams :" << std::endl );
	mTestPurposeClockParams.strFQN( AVM_OS_DEBUG << "mTestPurposeClockParams :" << std::endl );
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	addVariables(tcPropertyDecl, mTestPurposeInoutParams, mTestPurposeClockParams);

	AvmTestCaseUtils::getInitialVariablesOfParameters(
			rootEC, *mMachineTC, mNewfreshInitialVars);
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	mNewfreshInitialVars.strFQN( AVM_OS_DEBUG << "mNewfreshInitialVars :" << std::endl );
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	AvmTestCaseUtils::newfreshTraceVarsFromEC(
			*tpTargetEC, *mMachineTC, mNewfreshTraceVarsTP);
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	mNewfreshTraceVarsTP.strFQN( AVM_OS_DEBUG << "mNewfreshTraceVarsTP :" << std::endl );
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	mNewfreshInitialTraceVarsTP.append(mNewfreshInitialVars);
	mNewfreshInitialTraceVarsTP.add_unique(mNewfreshTraceVarsTP);
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	mNewfreshInitialTraceVarsTP.strFQN( AVM_OS_DEBUG << "mNewfreshInitialTraceVarsTP :" << std::endl );
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

//	buildStatemachine();

	buildStatemachineTC();
}


// Saving SUT port for saving in JSON format
static Port::Table OUTPUT_PORTS;
static Port::Table INPUT_PORTS;
static Port::Table UNCRNTROLLABLE_INPUT_PORTS;


void AvmTestCaseFactory::addPorts(PropertyPart & tcPropertyPart,
		InteractionPart * tcInteractionPart, const PropertyPart & sutPropertyPart)
{
	Modifier inputModifier;
	inputModifier.setVisibilityPublic().setDirectionInput();

	Modifier outputModifier;
	outputModifier.setVisibilityPublic().setDirectionOutput();

	PropertyPart::TableOfPort::const_iterator sutItBf =
			sutPropertyPart.getPorts().begin();

	PropertyPart::TableOfPort::const_ref_iterator sutItp =
			sutPropertyPart.getPorts().begin();
	PropertyPart::TableOfPort::const_ref_iterator sutEndItp =
			sutPropertyPart.getPorts().end();
	for( ; sutItp != sutEndItp ; ++sutItp, ++sutItBf )
	{
		Connector & aConnector = tcInteractionPart->appendConnector(
				ComProtocol::PROTOCOL_ENVIRONMENT_KIND);

		if( sutItp->getModifier().isDirectionInput() )
		{
			if( mUncontrollableTraceFilter.pass(sutItp->to< Port >()) )
			{
				UNCRNTROLLABLE_INPUT_PORTS.append(*sutItBf);

				Port * tcInputPort = new Port(tcPropertyPart,
						sutItp->getNameID(), IComPoint::IO_PORT_NATURE, inputModifier);
				tcInputPort->setParameters( sutItp->getParameters() );
				addType( sutItp->getParameters() );

				const BF & inPort = tcPropertyPart.saveOwnedElement(tcInputPort);

				mUncontrollableSUT_toInputTC.append(inPort);

				aConnector.appendComRoute(
						tcInputPort, Modifier::PROPERTY_INPUT_DIRECTION );
			}
			else
			{
				INPUT_PORTS.append(*sutItBf);

				Port * tcOutputPort = new Port(tcPropertyPart,
						sutItp->getNameID(), IComPoint::IO_PORT_NATURE, outputModifier);
				tcOutputPort->setParameters( sutItp->getParameters() );
				addType( sutItp->getParameters() );
				tcPropertyPart.saveOwnedElement(tcOutputPort);

				aConnector.appendComRoute(
						tcOutputPort, Modifier::PROPERTY_OUTPUT_DIRECTION);
			}
		}
		else if( sutItp->getModifier().isDirectionOutput() )
		{
			OUTPUT_PORTS.append(*sutItBf);

			Port * tcInputPort = new Port(tcPropertyPart,
					sutItp->getNameID(), IComPoint::IO_PORT_NATURE, inputModifier);
			tcInputPort->setParameters( sutItp->getParameters() );
			addType( sutItp->getParameters() );
			const BF & inPort = tcPropertyPart.saveOwnedElement(tcInputPort);

			mOutputPortSUT_toInputTC.append(inPort);

			aConnector.appendComRoute(
					tcInputPort, Modifier::PROPERTY_INPUT_DIRECTION);
		}
	}
}


void AvmTestCaseFactory::addType(const Variable::Table & portParameters)
{
	for( const auto & param : portParameters )
	{
		const Variable & paramVar = param.to< Variable >();
		if( paramVar.hasTypeSpecifier() )
		{
			addType(paramVar.getTypeSpecifier());
		}
		else if( paramVar.hasDataType() )
		{
			addType(paramVar.getDataType());
		}
	}
}

void AvmTestCaseFactory::addType(const BaseTypeSpecifier & paramType)
{
	if( paramType.hasAstElement() )
	{
		const DataType & dataType = paramType.getAstDataType();
		if( not dataType.hasTypeBasic() )
		{
			addType(dataType);
		}
	}
}

void AvmTestCaseFactory::addType(const DataType & dataType)
{
	if( not dataType.hasTypeBasic() )
	{
		if( mSystemTC.getDataType(dataType.getNameID()).invalid() )
		{
			mSystemTC.getPropertyPart().appendDataType(
					INCR_BF( const_cast< DataType * >(& dataType) ) );
		}
	}
}


void AvmTestCaseFactory::addVariables(PropertyPart & tcPropertyDecl,
		InstanceOfData::Table & tpInoutParameters,
		InstanceOfData::Table & tpClockParameters)
{
	ParametersRuntimeForm & paramsRF =
			mVarTC_subst_mParamTP_ED.getWritableParametersRuntimeForm();
	paramsRF.makeWritableDataTable();

	paramsRF.update(tpClockParameters);
	paramsRF.update(tpInoutParameters);


	// Variable Timeout declaration
	mVariable_TC_TM = tcPropertyDecl.saveOwnedElement(
			new Variable(mMachineTC,
					Modifier::PROPERTY_PRIVATE_MODIFIER,
					TypeManager::POS_RATIONAL, VAR_TM_ID) );

	avm_type_specifier_kind_t time_type_specifier =
			TimedMachine::timeTypeSpecifierKind(mSystemSUT.getSpecifier());
	const TypeSpecifier & aTimeDomain =
			TimedMachine::timeTypeSpecifier(mSystemSUT.getSpecifier());

	TypeSpecifier clockType(
			TypeManager::newClockTime(TYPE_CLOCK_SPECIFIER, aTimeDomain) );

	mVariable_TC_Clock = tcPropertyDecl.saveOwnedElement(
			new Variable(mMachineTC,
					Modifier::PROPERTY_PRIVATE_MODIFIER,
					clockType, VAR_TC_CLOCK_ID) );


	InstanceOfData::Table::const_raw_iterator itParam = tpInoutParameters.begin();
	InstanceOfData::Table::const_raw_iterator endParam = tpInoutParameters.end();
	for( ; itParam != endParam ; ++itParam )
	{
		const BaseTypeSpecifier & paramType = (itParam)->getTypeSpecifier();

		addType(paramType);

		BF typeVar = INCR_BF( const_cast< BaseTypeSpecifier * >(& paramType) );

		const BF tcVar = tcPropertyDecl.saveOwnedElement(
				new Variable(mMachineTC,
						Modifier::PROPERTY_PRIVATE_MODIFIER,
						typeVar, (itParam)->getNameID()) );

		// For substitution of symbex parameters by testcase variables
		(itParam)->getwModifier().setFeatureFinal( false );
		paramsRF.setData( (itParam)->getOffset(), tcVar );
	}

//	BF & timeElapsedType = mSystemSUT.getPropertyPart().getDeltaTimeType();
	TypeSpecifier timeElapsedType(
			TypeManager::newClockTime(time_type_specifier, aTimeDomain) );

	endParam = tpClockParameters.end();
	for( itParam = tpClockParameters.begin() ; itParam != endParam ; ++itParam )
	{
		const BaseTypeSpecifier & paramType = (itParam)->getTypeSpecifier();
		AVM_OS_ASSERT_FATAL_ERROR_EXIT( paramType.is< ContainerTypeSpecifier >() )
				<< "Unexpected a parameter variable < " << (itParam)->strHeaderId()
				<< " > without an time type as ContainerTypeSpecifier !" << std::endl
				<< SEND_EXIT;

//		TypeSpecifier timeElapsedType(
//				TypeManager::newClockTime(time_type_specifier,
//						paramType.to< ContainerTypeSpecifier
//							>().getContentsTypeSpecifier()) );
		const BF tcVar = tcPropertyDecl.saveOwnedElement(
				new Variable(mMachineTC,
						Modifier::PROPERTY_PRIVATE_MODIFIER,
						timeElapsedType, (itParam)->getNameID()) );

		// For substitution of symbex parameters by testcase variables
		(itParam)->getwModifier().setFeatureFinal( false );
		paramsRF.setData( (itParam)->getOffset(), tcVar );
	}
}


bool AvmTestCaseFactory::buildStatemachineTC()
{
	ExecutionContext::ListOfConstPtr traceECs(mTestPurposeTrace);
	const ExecutionContext * tcSourceEC = traceECs.pop_first();

	std::string stateID = (OSS() << "ec_" << tcSourceEC->getIdNumber());

	std::string stateName = (OSS() << "ec_" << tcSourceEC->getIdNumber()
		<< "_" << tcSourceEC->getExecutionData().strStateConf("%4%"));

	Machine * tcSourceState = Machine::newState(mMachineTC,
		stateID, Specifier::STATE_START_MOC, stateName);

	mMachineTC->saveOwnedElement(tcSourceState);

	for( const auto tcTargetEC : traceECs )
	{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << "Build state-transition for : " << tcSourceEC->str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

		Specifier stateSpec = Specifier::STATE_SIMPLE_MOC;

		tcSourceState = buildStep(*tcSourceState, *tcSourceEC, *tcTargetEC);

		tcSourceEC = tcTargetEC;
	}

	return true;
}

Machine * AvmTestCaseFactory::buildStep(Machine & tcSourceState,
		const ExecutionContext & tcSourceEC, const ExecutionContext & tcTargetEC)
{
	Machine * tcTargetState = nullptr;

	Port::Table unexpectedOutputSUT( mOutputPortSUT_toInputTC );
	Port::Table uncontrollableSUT( mUncontrollableSUT_toInputTC );

	// Test purpose EC
	const BF & ioTrace = tcTargetEC.getIOElementTrace();
	const BFCode & comTrace = BaseEnvironment::searchTraceIO(ioTrace);
	const InstanceOfPort & comPort = comTrace->first().to< InstanceOfPort >();

	if( StatementTypeChecker::isOutput(comTrace) )
	{
		if( tcTargetEC.hasChildContext() )
		{
			tcTargetState = applyRule_R02_Progress_SpecifiedOutput(
					tcSourceState, tcSourceEC, comTrace, tcTargetEC);
		}
		else
		{
			tcTargetState = applyRule_R04_Pass_SpecifiedOutput(
					tcSourceState, tcSourceEC, comTrace, tcTargetEC);
		}

		if( unexpectedOutputSUT.getByNameID(comPort.getNameID()).valid() )
		{
			applyRule_R10a_Fail_UnspecifiedOutput(
					tcSourceState, tcSourceEC, comTrace, tcTargetEC);

			unexpectedOutputSUT.removeByNameID(comPort.getNameID());
		}
	}
	// if( StatementTypeChecker::isInput(comTrace) )
	else if( mUncontrollableTraceFilter.pass(comPort) )
	{
		if( uncontrollableSUT.getByNameID(comPort.getNameID()).valid() )
		{
			tcTargetState = applyRule_R03_Progress_UncontrollableInput_Specified(
					tcSourceState, tcSourceEC, comTrace, tcTargetEC);

//			uncontrollableSUT.removeByNameID(comPort.getNameID());
		}
	}
	else
	{
		tcTargetState = applyRule_R01_Progress_Stimulation(
				tcSourceState, tcSourceEC, comTrace, tcTargetEC);
	}

	// Quiescence : Admissible
	applyRule_R09_Inconclusive_SpecifiedQuiescence_Admissible(
			tcSourceState, tcSourceEC, tcTargetEC);

	// Quiescence : Unspecified
	applyRule_R11_Fail_UnspecifiedQuiescence(
			tcSourceState, tcSourceEC, tcTargetEC);

	// Sibling test purpose EC
	for( const auto & aChildEC : tcSourceEC.getChildContexts()  )
	{
		if( aChildEC == (& tcTargetEC) )
		{
			continue;
		}

AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << "Build sibling-transition for child EC : " << aChildEC->str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

		if( (not aChildEC->hasIOElementTrace())
			|| (not aChildEC->hasRunnableElementTrace() ) )
		{
			continue;
		}
		const BF & ioTrace = aChildEC->getIOElementTrace();
		const BFCode & comTrace = BaseEnvironment::searchTraceIO(ioTrace);
		const InstanceOfPort & comPort = comTrace->first().to< InstanceOfPort >();

		if( StatementTypeChecker::isOutput(comTrace) )
		{
			applyRule_R06_Inconclusive_SpecifiedOutput(
					tcSourceState, tcSourceEC, comTrace, *aChildEC);

			if( unexpectedOutputSUT.getByNameID(comPort.getNameID()).valid() )
			{
				applyRule_R10a_Fail_UnspecifiedOutput(
						tcSourceState, tcSourceEC, comTrace, *aChildEC);

				unexpectedOutputSUT.removeByNameID(comPort.getNameID());
			}
		}
		// if( StatementTypeChecker::isInput(comTrace) )
		else if( mUncontrollableTraceFilter.pass(comPort) )
		{
			applyRule_R07_Inconclusive_UncontrollableInput_Specified(
					tcSourceState, tcSourceEC, comTrace, *aChildEC);
		}
//		else
//		{
//			if( aChildEC->getFlags().hasCoverageElement() )
//			{
//				applyRule_R06_Inconclusive_UncontrollableInput_Specified(
//						tcSourceState, tcSourceEC, comTrace, *aChildEC);
//			}
//			else
//			{
//				applyRule_R07_Inconclusive_UncontrollableInput_unspecified(
//						tcSourceState, tcSourceEC, comTrace, *aChildEC);
//			}
//		}
	}

	for( const auto & ucInPort : uncontrollableSUT )
	{
		applyRule_R08_Inconclusive_UncontrollableInput_unspecified(
				tcSourceState, tcSourceEC, ucInPort, tcTargetEC);
	}

	for( const auto & obsPort : unexpectedOutputSUT )
	{
		applyRule_R10b_Fail_UnspecifiedOutput(
				tcSourceState, tcSourceEC, obsPort, tcTargetEC);
	}

	return tcTargetState;
}


////////////////////////////////////////////////////////////////////////////////
// RULES FOR TESCASE GENERATION
////////////////////////////////////////////////////////////////////////////////

BF AvmTestCaseFactory::boundTimeOutCondition(const ExecutionContext & tcSourceEC)
{
	// The time elapsed value : z_i
	const BF & varElapsedTime =
			AvmTestCaseUtils::newfreshDurationVarFromEC(tcSourceEC, *mMachineTC);

	// Time elapsed constraint
	return ExpressionConstructor::andExpr(
			ExpressionConstructor::ltExpr(mVariable_TC_Clock, mVariable_TC_TM),
			ExpressionConstructor::eqExpr(varElapsedTime, mVariable_TC_Clock) );
}

BF AvmTestCaseFactory::targetPathCondition(const ExecutionContext & tcTargetEC)
{
	BF guardCondition = tcTargetEC.getPathCondition();
	if( (not guardCondition.isEqualTrue()) and (not mNewfreshInitialVars.empty()) )
	{
		guardCondition = ExpressionConstructor::existsExpr(
				mNewfreshInitialVars, tcTargetEC.getPathCondition());
	}

	return guardCondition;
}

BF AvmTestCaseFactory::unboundTimeOutCondition(const ExecutionContext & tcSourceEC)
{
	// The time elapsed value : z_i
	const BF & varElapsedTime =
			AvmTestCaseUtils::newfreshDurationVarFromEC(tcSourceEC, *mMachineTC);

	// Time elapsed constraint
	return ExpressionConstructor::andExpr(
			ExpressionConstructor::gteExpr(mVariable_TC_Clock, mVariable_TC_TM),
			ExpressionConstructor::eqExpr(varElapsedTime, mVariable_TC_Clock) );
}


// PROGRESS
Machine * AvmTestCaseFactory::applyRule_R01_Progress_Stimulation(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BFCode & comTrace, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R01_Progress_Stimulation for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tTrace : " << comTrace.str() << std::endl
		<< "\ttp_PC : " << mTestPurposePathCondition.str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	// The target state on the test purpose path
	std::string stateID = (OSS() << "ec_" << tcTargetEC.getIdNumber());

	std::string stateName = (OSS() << "ec_" << tcTargetEC.getIdNumber()
		<< "_" << tcTargetEC.getExecutionData().strStateConf("%4%"));

	Machine * tcTargetState = Machine::newState(mMachineTC,
		stateID, Specifier::STATE_SIMPLE_MOC, stateName);

	mMachineTC->saveOwnedElement(tcTargetState);

	const std::string & portID =
			comTrace->first().to< InstanceOfPort >().getNameID();

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R1_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BF guardCondition = ExpressionConstant::BOOLEAN_TRUE;
	if( not mTestPurposePathCondition.isEqualTrue() )
	{
		Variable::Table boundVars( mNewfreshInitialTraceVarsTP );

		Variable::Table newfreshTraceVarsTargetEC;
		AvmTestCaseUtils::newfreshTraceVarsFromEC(
				tcSourceEC, *mMachineTC, newfreshTraceVarsTargetEC);

AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
		newfreshTraceVarsTargetEC.strFQN( AVM_OS_DEBUG << "newfreshVarsTargetEC :" << std::endl );
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

//		existsBoundVars.remove(F_vars(TargetEC));
		for( const auto & freshVarTargetEC : newfreshTraceVarsTargetEC )
		{
			boundVars.remove(freshVarTargetEC);
		}

AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	boundVars.strFQN( AVM_OS_DEBUG << "existsBoundVars :" << std::endl );
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

		guardCondition = ExpressionConstructor::andExpr(
				guardCondition,
				boundVars.empty() ?
						mTestPurposePathCondition :
						ExpressionConstructor::existsExpr(
								boundVars,
								mTestPurposePathCondition
						)
		);
	}

	BFCode timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD, boundTimeOutCondition(tcSourceEC) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD, guardCondition);

	// Statistic collector
	mTestCaseStatistics.takeAccount(guardCondition, tpTransition);

	// The Stimulation com statement
	BFCode tcStimulationComStatement =
			AvmTestCaseUtils::tpTrace_to_tcStatement(*mMachineTC, comTrace);

	// The reset of the testcase clock
	BFCode tcClockReset = StatementConstructor::newCode(
			OperatorManager::OPERATOR_ASSIGN, mVariable_TC_Clock,
			ExpressionConstant::INTEGER_ZERO);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, guard, tcStimulationComStatement, tcClockReset));

	return tcTargetState;
}

// PROGRESS
Machine * AvmTestCaseFactory::applyRule_R02_Progress_SpecifiedOutput(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BFCode & comTrace, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R02_Progress_SpecifiedOutput for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tTrace : " << comTrace.str() << std::endl
		<< "\tPC    : " << tcTargetEC.getPathCondition().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	// The target state on the test purpose path
	std::string stateID = (OSS() << "ec_" << tcTargetEC.getIdNumber());

	std::string stateName = (OSS() << "ec_" << tcTargetEC.getIdNumber()
		<< "_" << tcTargetEC.getExecutionData().strStateConf("%4%"));

	Machine * tcTargetState = Machine::newState(mMachineTC,
		stateID, Specifier::STATE_SIMPLE_MOC, stateName);

	mMachineTC->saveOwnedElement(tcTargetState);

	const std::string & portID =
			comTrace->first().to< InstanceOfPort >().getNameID();

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R2_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode 	timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			boundTimeOutCondition(*(tcTargetEC.getContainer())) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD,
			targetPathCondition(tcTargetEC) );

	// Statistic collector
	mTestCaseStatistics.takeAccount(guard->first(), tpTransition);

	// The Observation com statement
	BFCode tcObservationComStatement =
			AvmTestCaseUtils::tpTrace_to_tcStatement(*mMachineTC, comTrace);

	// The reset of the testcase clock
	BFCode tcClockReset = StatementConstructor::newCode(
			OperatorManager::OPERATOR_ASSIGN, mVariable_TC_Clock,
			ExpressionConstant::INTEGER_ZERO);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard, tcClockReset));

	return tcTargetState;
}

// PROGRESS
Machine * AvmTestCaseFactory::applyRule_R03_Progress_UncontrollableInput_Specified(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BFCode & comTrace, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R03_Progress_UncontrollableInput_Specified for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tTrace : " << comTrace.str() << std::endl
		<< "\tPC   : " << tcTargetEC.getPathCondition().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	// The target state on the test purpose path
	std::string stateID = (OSS() << "ec_" << tcTargetEC.getIdNumber());

	std::string stateName = (OSS() << "ec_" << tcTargetEC.getIdNumber()
		<< "_" << tcTargetEC.getExecutionData().strStateConf("%4%"));

	Machine * tcTargetState = Machine::newState(mMachineTC,
		stateID, Specifier::STATE_SIMPLE_MOC, stateName);

	mMachineTC->saveOwnedElement(tcTargetState);

	const std::string & portID =
			comTrace->first().to< InstanceOfPort >().getNameID();

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R3_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode 	timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			boundTimeOutCondition(*(tcTargetEC.getContainer())) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD,
			targetPathCondition(tcTargetEC) );

	// Statistic collector
	mTestCaseStatistics.takeAccount(guard->first(), tpTransition);

	// The Observation com statement
	BFCode tcObservationComStatement =
			AvmTestCaseUtils::tpTrace_to_tcStatement(*mMachineTC, comTrace);

	// The reset of the testcase clock
	BFCode tcClockReset = StatementConstructor::newCode(
			OperatorManager::OPERATOR_ASSIGN, mVariable_TC_Clock,
			ExpressionConstant::INTEGER_ZERO);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard, tcClockReset));

	return tcTargetState;
}

// PROGRESS --> PASS
Machine * AvmTestCaseFactory::applyRule_R04_Pass_SpecifiedOutput(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BFCode & comTrace, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R04_Pass_SpecifiedOutput for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tTrace : " << comTrace.str() << std::endl
		<< "\tPC   : " << tcTargetEC.getPathCondition().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	// The target state PASS of the test purpose
	std::string stateID = (OSS() << "PASS_ec_" << tcTargetEC.getIdNumber());

	std::string stateName = (OSS() << "PASS_ec_" << tcTargetEC.getIdNumber()
		<< "_" << tcTargetEC.getExecutionData().strStateConf("%4%"));

	Machine * tcTargetState = Machine::newState(mMachineTC,
		stateID, Specifier::STATE_FINAL_MOC, stateName);

	mMachineTC->saveOwnedElement(tcTargetState);

	const std::string & portID =
			comTrace->first().to< InstanceOfPort >().getNameID();

	// The transition PASS of the test purpose
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R4_PASS_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode 	timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			boundTimeOutCondition(*(tcTargetEC.getContainer())) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD,
			targetPathCondition(tcTargetEC) );

	// Statistic collector
	mTestCaseStatistics.takeAccount(guard->first(), tpTransition);

	// The Observation com statement
	BFCode tcObservationComStatement =
			AvmTestCaseUtils::tpTrace_to_tcStatement(*mMachineTC, comTrace);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard));

	return tcTargetState;
}

Machine * AvmTestCaseFactory::applyRule_R05_Pass_SpecifiedQuiescence(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BFCode & comTrace, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R05_Pass_SpecifiedQuiescence for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tTrace : " << comTrace.str() << std::endl
		<< "\tPC   : " << tcTargetEC.getPathCondition().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	// The target state PASS of the test purpose
	std::string stateID = (OSS() << "PASS_ec_" << tcTargetEC.getIdNumber());

	std::string stateName = (OSS() << "PASS_ec_" << tcTargetEC.getIdNumber()
		<< "_" << tcTargetEC.getExecutionData().strStateConf("%4%"));

	Machine * tcTargetState = Machine::newState(mMachineTC,
		stateID, Specifier::STATE_FINAL_MOC, stateName);

	mMachineTC->saveOwnedElement(tcTargetState);

	const std::string & portID =
			comTrace->first().to< InstanceOfPort >().getNameID();

	// The transition PASS of the test purpose
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R5_PASS_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			unboundTimeOutCondition(tcSourceEC) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD, targetPathCondition(tcTargetEC) );

	// Statistic collector
	mTestCaseStatistics.takeAccount(guard->first(), tpTransition);

	BFCode tcObservationComStatement =
			AvmTestCaseUtils::tpTrace_to_tcStatement(*mMachineTC, comTrace);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard));

	return tcTargetState;
}


// INCONCLUSIVE OUTPUT
void AvmTestCaseFactory::applyRule_R06_Inconclusive_SpecifiedOutput(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BFCode & comTrace, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R06_Inconclusive_SpecifiedOutput for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tTrace : " << comTrace.str() << std::endl
		<< "\tPC   : " << tcTargetEC.getPathCondition().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	const std::string & portID =
			comTrace->first().to< InstanceOfPort >().getNameID();

	// The target state on the test purpose path
	std::string stateID = (OSS() << "INC_out_ec_" << tcSourceEC.getIdNumber()
			<< "_" << tcTargetEC.getIdNumber() << "_" << portID);
	Machine * tcTargetState = Machine::newState(mMachineTC,
			stateID, Specifier::PSEUDOSTATE_TERMINAL_MOC);
	mMachineTC->saveOwnedElement(tcTargetState);

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R6_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode 	timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			boundTimeOutCondition(*(tcTargetEC.getContainer())) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD,
			targetPathCondition(tcTargetEC) );

	// Statistic collector
	mTestCaseStatistics.takeAccount(guard->first(), tpTransition);

	// The Observation com statement
	BFCode tcObservationComStatement =
			AvmTestCaseUtils::tpTrace_to_tcStatement(*mMachineTC, comTrace);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard));
}

// INCONCLUSIVE UNCONTROLLABLE INPUT SPECIFIED
void AvmTestCaseFactory::applyRule_R07_Inconclusive_UncontrollableInput_Specified(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BFCode & comTrace, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R07_Inconclusive_UncontrollableInput_Specified for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tTrace : " << comTrace.str() << std::endl
		<< "\tPC   : " << tcTargetEC.getPathCondition().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	const std::string & portID =
			comTrace->first().to< InstanceOfPort >().getNameID();

	// The target state on the test purpose path
	std::string stateID = (OSS() << "INC_ucInSpec_ec_" << tcSourceEC.getIdNumber()
			<< "_" << tcTargetEC.getIdNumber() << "_" << portID);
	Machine * tcTargetState = Machine::newState(mMachineTC,
			stateID, Specifier::PSEUDOSTATE_TERMINAL_MOC);
	mMachineTC->saveOwnedElement(tcTargetState);

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R7_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode 	timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			boundTimeOutCondition(*(tcTargetEC.getContainer())) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD,
			targetPathCondition(tcTargetEC) );

	// Statistic collector
	mTestCaseStatistics.takeAccount(guard->first(), tpTransition);

	// The Observation com statement
	BFCode tcObservationComStatement =
			AvmTestCaseUtils::tpTrace_to_tcStatement(*mMachineTC, comTrace);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard));
}

// INCONCLUSIVE UNCONTROLLABLE INPUT UNSPECIFIED
void AvmTestCaseFactory::applyRule_R08_Inconclusive_UncontrollableInput_unspecified(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BF & ucInPort, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R08_Inconclusive_UncontrollableInput_unspecified for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tucInPort : " << ucInPort.str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	const Port & ucInPortTC = ucInPort.to< Port >();

	// The guard
	BFCode ucontrollableInputGuards = ExpressionConstructor::newCode(
			OperatorManager::OPERATOR_AND );
	for( const auto & aChildEC : tcSourceEC.getChildContexts() )
	{
		if( aChildEC->hasIOElementTrace() && aChildEC->hasRunnableElementTrace() )
		{
			const BF & ioTrace = aChildEC->getIOElementTrace();
			const BFCode & specComTrace = BaseEnvironment::searchTraceIO(ioTrace);

			if( StatementTypeChecker::isInput(specComTrace) )
			{
				const InstanceOfPort & ucinSpecPort =
						specComTrace->first().to< InstanceOfPort >();
				if( ucInPortTC.getNameID() == ucinSpecPort.getNameID() )
				{
					ucontrollableInputGuards.append(
							ExpressionConstructor::notExpr(
									targetPathCondition(*aChildEC) ) );

//					if( aChildEC->getPathCondition().isEqualTrue() )
//					{
//						return;
//					}
				}
			}
		}
	}

	const std::string & portID = ucInPortTC.getNameID();

	// The target state on the test purpose path
	std::string stateID = (OSS() << "INC_ucInUspec_ec_"
			<< tcSourceEC.getIdNumber() << "_" << portID);
	Machine * tcTargetState = mMachineTC->getMachineByNameID(stateID);
	if( tcTargetState == nullptr )
	{
		tcTargetState = Machine::newState(mMachineTC,
				stateID, Specifier::PSEUDOSTATE_TERMINAL_MOC);
		mMachineTC->saveOwnedElement(tcTargetState);
	}

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R8_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			boundTimeOutCondition(tcSourceEC) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD,
			( ucontrollableInputGuards.size() > 1 )
					? ucontrollableInputGuards
					: ( ucontrollableInputGuards.size() > 0 )
					  	? ucontrollableInputGuards->first()
						: ExpressionConstant::BOOLEAN_TRUE );

	// Statistic collector
	mTestCaseStatistics.takeAccount(ucontrollableInputGuards, tpTransition);

	// The Observation com statement
	BFCode tcObservationComStatement = StatementConstructor::newCode(
			OperatorManager::OPERATOR_INPUT, ucInPort);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard));
}


// INCONCLUSIVE SPECIFIED QUIESCENCE ADMISSIBLE
void AvmTestCaseFactory::applyRule_R09_Inconclusive_SpecifiedQuiescence_Admissible(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R09_Inconclusive_SpecifiedQuiescence_Admissible for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	BF quiescenceCondition = compute_R09_QuiescenceCondition(tcSourceEC);

AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << "Quiescence condition " << quiescenceCondition.str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	if( quiescenceCondition.isEqualFalse() )
	{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << "Quiescence condition is FALSE ! " << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
//		return;
	}

	// The target state on the test purpose path
	std::string stateID = (OSS() << "INC_dur_ec_" << tcSourceEC.getIdNumber() );
	Machine * tcTargetState = Machine::newState(mMachineTC,
			stateID, Specifier::PSEUDOSTATE_TERMINAL_MOC);
	mMachineTC->saveOwnedElement(tcTargetState);

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R9_quiescence", Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			unboundTimeOutCondition(tcSourceEC) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD, quiescenceCondition);

	// Statistic collector
	mTestCaseStatistics.takeAccount(quiescenceCondition, tpTransition);

	// The Quiescence Observation com statement
	BFCode failQuiescenceStatement = StatementConstructor::newCode(
			OperatorManager::OPERATOR_INPUT, mQuiescencePortTC);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, failQuiescenceStatement, guard));
}

BF AvmTestCaseFactory::compute_R09_QuiescenceCondition(
		const ExecutionContext & tcSourceEC)
{
	AvmCode::OperandCollectionT phiQuiescence;

	for( const auto & aChildEC : tcSourceEC.getChildContexts() )
	{
AVM_IF_DEBUG_LEVEL_FLAG2( MEDIUM , PROCESSING , TEST_DECISION )
	AVM_OS_DEBUG << "R9_Quiescence condition for " << aChildEC->str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG2( MEDIUM , PROCESSING , TEST_DECISION )

		if( aChildEC->hasIOElementTrace() )
		{
			const BF & ioTrace = aChildEC->getIOElementTrace();
			const BFCode & comTrace = BaseEnvironment::searchTraceIO(ioTrace);
			const InstanceOfPort & comPort =
					comTrace->first().to< InstanceOfPort >();
			if( comPort.isTEQ(mQuiescencePortTP.rawPort()) )
			{
				if( not aChildEC->getPathCondition().isEqualTrue() )
				{
					phiQuiescence.append(
							ExpressionConstructor::existsExpr(
									mNewfreshInitialVars,
									aChildEC->getPathCondition()
							)
					);
AVM_IF_DEBUG_LEVEL_FLAG2( MEDIUM , PROCESSING , TEST_DECISION )
	AVM_OS_DEBUG << "R9_Quiescence condition for output< " << comPort.getNameID()
		<< " > of " << aChildEC->str() << " : ";
AVM_ENDIF_DEBUG_LEVEL_FLAG2( MEDIUM , PROCESSING , TEST_DECISION )
				}
			}
		}
	}

	if( phiQuiescence.nonempty() )
	{
		return ExpressionConstructor::orExpr(phiQuiescence);
	}
	else
	{
		return( ExpressionConstant::BOOLEAN_TRUE );
	}
}


// FAIL UNSPECIFIED OUTPUT
void AvmTestCaseFactory::applyRule_R10a_Fail_UnspecifiedOutput(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BFCode & comTrace, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R10a_Fail_UnspecifiedOutput for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tTrace : " << comTrace.str() << std::endl
		<< "\tPC   : " << tcTargetEC.getPathCondition().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	const InstanceOfPort & failOutPort = comTrace->first().to< InstanceOfPort >();
	// The guard
	BFCode outputGuards = StatementConstructor::newCode(
			OperatorManager::OPERATOR_AND );
	for( const auto & aChildEC : tcSourceEC.getChildContexts() )
	{
		if( aChildEC->hasIOElementTrace() && aChildEC->hasRunnableElementTrace() )
		{
			const BF & ioTrace = aChildEC->getIOElementTrace();
			const BFCode & specComTrace = BaseEnvironment::searchTraceIO(ioTrace);

			if( StatementTypeChecker::isOutput(specComTrace) )
			{
				const InstanceOfPort & specOutPort =
						specComTrace->first().to< InstanceOfPort >();
				if( (& failOutPort) == (& specOutPort) )
				{
					outputGuards.append(
							ExpressionConstructor::notExpr(
									targetPathCondition(*aChildEC) ) );

//					if( aChildEC->getPathCondition().isEqualTrue() )
//					{
//						return;
//					}
				}
			}
		}
	}

	const std::string & portID = failOutPort.getNameID();

	// The target state on the test purpose path
	std::string stateID = (OSS()<< "FAIL_out_ec_" << tcSourceEC.getIdNumber()
			<< "_" << tcTargetEC.getIdNumber() << "_" << portID);
	Machine * tcTargetState = mMachineTC->getMachineByNameID(stateID);
	if( tcTargetState == nullptr )
	{
		tcTargetState = Machine::newState(mMachineTC,
				stateID, Specifier::PSEUDOSTATE_TERMINAL_MOC);
		mMachineTC->saveOwnedElement(tcTargetState);
	}

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R10a_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			boundTimeOutCondition(tcSourceEC) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD,
			( outputGuards.size() > 1 )
				? outputGuards
				: ( outputGuards.size() > 0 )
					? outputGuards->first()
					: ExpressionConstant::BOOLEAN_TRUE);


	// Statistic collector
	mTestCaseStatistics.takeAccount(outputGuards, tpTransition);

	// The Observation com statement
	BFCode tcObservationComStatement =
			AvmTestCaseUtils::tpTrace_to_tcStatement(*mMachineTC, comTrace);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard));
}


// FAIL UNSPECIFIED OUTPUT
void AvmTestCaseFactory::applyRule_R10b_Fail_UnspecifiedOutput(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const BF & obsPort, const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R10b_Fail_UnspecifiedOutput for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tobsPort : " << obsPort.str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	const Port & inputPortTC = obsPort.to< Port >();

	// The guard
	BFCode unspecifiedOutputGuards = StatementConstructor::newCode(
			OperatorManager::OPERATOR_AND );
	for( const auto & aChildEC : tcSourceEC.getChildContexts() )
	{
		if( aChildEC->hasIOElementTrace() && aChildEC->hasRunnableElementTrace() )
		{
			const BF & ioTrace = aChildEC->getIOElementTrace();
			const BFCode & specComTrace = BaseEnvironment::searchTraceIO(ioTrace);

			if( StatementTypeChecker::isInput(specComTrace) )
			{
				const InstanceOfPort & ucinSpecPort =
						specComTrace->first().to< InstanceOfPort >();
				if( inputPortTC.getNameID() == ucinSpecPort.getNameID() )
				{
					unspecifiedOutputGuards.append(
							ExpressionConstructor::notExpr(
									targetPathCondition(*aChildEC) ) );

//					if( aChildEC->getPathCondition().isEqualTrue() )
//					{
//						return;
//					}
				}
			}
		}
	}

	const std::string & portID = obsPort.to< Port >().getNameID();

	// The target state on the test purpose path
	std::string stateID = (OSS()<< "FAIL_out_ec_" << tcSourceEC.getIdNumber()
			<< "_" << portID);
	Machine * tcTargetState = mMachineTC->getMachineByNameID(stateID);
	if( tcTargetState == nullptr )
	{
		tcTargetState = Machine::newState(mMachineTC,
				stateID, Specifier::PSEUDOSTATE_TERMINAL_MOC);
		mMachineTC->saveOwnedElement(tcTargetState);
	}

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R10b_" + portID, Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			boundTimeOutCondition(tcSourceEC) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD,
			( unspecifiedOutputGuards.size() > 1 )
				? unspecifiedOutputGuards
				: ( unspecifiedOutputGuards.size() > 0 )
					? unspecifiedOutputGuards->first()
					: ExpressionConstant::BOOLEAN_TRUE);

	// Statistic collector
	mTestCaseStatistics.takeAccount(unspecifiedOutputGuards, tpTransition);

	// The Observation com statement
	BFCode tcObservationComStatement = StatementConstructor::newCode(
			OperatorManager::OPERATOR_INPUT, obsPort);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, tcObservationComStatement, guard));
}

// FAIL DURATION : UNSPECIFIED QUIESCENCE
void AvmTestCaseFactory::applyRule_R11_Fail_UnspecifiedQuiescence(
		Machine & tcSourceState, const ExecutionContext & tcSourceEC,
		const ExecutionContext & tcTargetEC)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << std::endl << EMPHASIS( "applyRule_R11_Fail_UnspecifiedQuiescence for " )
		<< "\tSource EC : " << tcSourceEC.str() << std::endl
		<< "\tTarget EC : " << tcTargetEC.str() << std::endl
		<< "\tPC   : " << tcTargetEC.getPathCondition().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	BF quiescenceCondition = compute_R11_QuiescenceCondition(tcSourceEC);

AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << "Quiescence condition " << quiescenceCondition.str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )

	if( quiescenceCondition.isEqualFalse() )
	{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
	AVM_OS_DEBUG << "Quiescence condition is FALSE ! " << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , PROCESSING )
//		return;
		quiescenceCondition = ExpressionConstant::BOOLEAN_FALSE;
	}

	// The target state on the test purpose path
	std::string stateID = (OSS() << "FAIL_dur_ec_" << tcSourceEC.getIdNumber() );
	Machine * tcTargetState = Machine::newState(mMachineTC,
			stateID, Specifier::PSEUDOSTATE_TERMINAL_MOC);
	mMachineTC->saveOwnedElement(tcTargetState);

	// The transition on the test purpose path
	Transition * tpTransition = new Transition(tcSourceState,
			"tr_R11_unspecifiedQuiescence", Transition::MOC_SIMPLE_KIND);
	tpTransition->setTarget( *tcTargetState );
	tcSourceState.saveOwnedElement( tpTransition );

	// The guard
	BFCode timedGuard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_TIMED_GUARD,
			unboundTimeOutCondition(tcSourceEC) );

	BFCode guard = StatementConstructor::newCode(
			OperatorManager::OPERATOR_GUARD, quiescenceCondition);

	// Statistic collector
	mTestCaseStatistics.takeAccount(quiescenceCondition, tpTransition);

	// The Quiescence Observation com statement
	BFCode failQuiescenceStatement = StatementConstructor::newCode(
			OperatorManager::OPERATOR_INPUT, mQuiescencePortTC);

	tpTransition->setStatement( StatementConstructor::newCode(
			OperatorManager::OPERATOR_SEQUENCE,
			timedGuard, failQuiescenceStatement, guard));
}


BF AvmTestCaseFactory::compute_R11_QuiescenceCondition(
		const ExecutionContext & tcSourceEC)
{
	AvmCode::OperandCollectionT phiQuiescence;

	for( const auto & aChildEC : tcSourceEC.getChildContexts() )
	{
AVM_IF_DEBUG_LEVEL_FLAG2( MEDIUM , PROCESSING , TEST_DECISION )
	AVM_OS_DEBUG << "R11_Quiescence condition for " << aChildEC->str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG2( MEDIUM , PROCESSING , TEST_DECISION )

		if( aChildEC->hasIOElementTrace() )
		{
			const BF & ioTrace = aChildEC->getIOElementTrace();
			const BFCode & comTrace = BaseEnvironment::searchTraceIO(ioTrace);
			const InstanceOfPort & comPort =
					comTrace->first().to< InstanceOfPort >();

			if( comPort.isTEQ(mQuiescencePortTP.rawPort()) )
			{
				phiQuiescence.append(
						ExpressionConstructor::forallExpr(
								mNewfreshInitialVars,
								ExpressionConstructor::notExpr(
										aChildEC->getPathCondition()
								)
						)
				);
AVM_IF_DEBUG_LEVEL_FLAG2( MEDIUM , PROCESSING , TEST_DECISION )
	AVM_OS_DEBUG << "R11_Quiescence condition for < " << comPort.getNameID() << " > of "
		<< aChildEC->str() << " : " << phiQuiescence.last().str() << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG2( MEDIUM , PROCESSING , TEST_DECISION )
			}
		}
	}
	if( phiQuiescence.nonempty() )
	{
		return ExpressionConstructor::andExpr(phiQuiescence);
	}
	else
	{
		return( ExpressionConstant::BOOLEAN_FALSE );
	}
}



////////////////////////////////////////////////////////////////////////////////
// Saving Testcase in JSON format
////////////////////////////////////////////////////////////////////////////////
//static const Port::Table OUTPUT_PORTS;
//static const Port::Table INPUT_PORTS;
//static const Port::Table UNCRNTROLLABLE_INPUT_PORTS;

static const std::string TC_MANIFEST =
		"\n\t\"manifest\": {"
		"\n\t\t\"version\": 0.1,"
		"\n\t\t\"description\": \"Generated testcases definition in JSON Format\","
		"\n\t\t\"service\": \"Testcase Specification\","
		"\n\t\t\"generatedDate\": \"" + ExecutionChrono::current_time() + "\""
		"\n\t},\n";

static void sutPortToJson(OutStream & out, const Port & port)
{
	out << EOL << TAB4 << "{"
		<< EOL << TAB5 << "\"name\": \"" << port.getNameID() << "\"";
	if( not port.getParameters().empty() )
	{
		out << ",";
		out<< EOL << TAB5 << "\"parameters\": [";

		TableOfVariable::const_raw_iterator itParam = port.getParameters().begin();
		TableOfVariable::const_raw_iterator endIt = port.getParameters().end();
		std::string commaSep = "";
		for( std::size_t offset = 0 ; itParam != endIt ; ++itParam , ++offset)
		{
			out << commaSep;
			std::string paramName = itParam->getNameID();
			if( paramName.empty() )
			{
				paramName = "$" + to_string(offset) ;
			}
			out << EOL << TAB6 << "{"
				<< EOL << TAB7 << "\"name\": \"" << paramName << "\","
				<< EOL << TAB7 << "\"type\": \"" << itParam->strTypeSpecifier() << "\""
				<< EOL << TAB6 << "}";
			commaSep = ",";
		}
		out << EOL << TAB5 << "]";
	}
	out << EOL << TAB4 << "}";
}

static std::string tcVerdictToJson(const std::string & stateName)
{
	if( stateName.starts_with("FAIL_out") )
	{
		return "FAIL_OUTPUT";
	}
	else if( stateName.starts_with("INC_out") )
	{
		return "INCONCLUSIVE_OUTPUT ";
	}

	else if( stateName.starts_with("FAIL_dur") )
	{
		return "FAIL_DURATION";
	}
	else if( stateName.starts_with("INC_dur") )
	{
		return "INCONCLUSIVE_DURATION ";
	}

	else if( stateName.starts_with("INC_ucInSpec") )
	{
		return "INCONCLUSIVE_UNCROLLABLE_INPUT_SPECIFIED";
	}
	else if( stateName.starts_with("INC_ucInUspec") )
	{
		return "INCONCLUSIVE_UNCROLLABLE_INPUT_UNSPECIFIED";
	}

	else if( stateName.starts_with("PASS") )
	{
		return "PASS";
	}

	return stateName;
}

static std::string tcTimeVariableToJson(const Machine & state)
{
	BFCode timedGuard;
	Variable::Table timeVars;
	for( const auto & itTransition :
			state.getBehaviorPart()->getOutgoingTransitions() )
	{
		StatementFactory::collectTimedGuard(
				itTransition.to< Transition >().getStatement(), timedGuard);
		if( timedGuard.valid() )
		{
			ExpressionFactory::collectSpecVariable(timedGuard, timeVars);
			for( const auto & itVar : timeVars )
			{
				const Variable & timeVar = itVar.to< Variable >();
				if( (timeVar.getNameID() != VAR_TC_CLOCK_ID)
						and (timeVar.getNameID() != VAR_TM_ID))
				{
					return timeVar.getNameID();
				}
				else if( timeVar.getNameID().starts_with(PropertyPart::VAR_ID_DELTA_TIME) )
				{
					return timeVar.getNameID();
				}
			}
		}
	}

	return "UNFOUND_TIME_VARIABLE";
}

static void tcTransitionToJson(OutStream & out,
		const Transition & transition, const BF & quiescencePortTC)
{
	BFCode guard;
	BFCode comStatement;

	StatementFactory::collectGuardCommunication(
			transition.getStatement(), guard, comStatement);

	StringOutStream outSMT( AVM_STR_INDENT );
//	StringOutStream outSMT( AVM_TAB_INDENT );
	BFVector paramVector;
	Z3Solver aSolver;
//	aSolver.to_smtlib(outSMT, guard->first());

	aSolver.to_smt(outSMT << EOL, guard->first(), paramVector);
//	SolverFactory::to_smt(outSMT, guard, SolverDef::SOLVER_Z3_KIND, false);

	std::string nature;
	const Port & port = comStatement->first().to< Port >();
	if( quiescencePortTC.raw_pointer() == &port )
	{
		nature = "QUIESCENCE";
	}
	else if( StatementTypeChecker::isOutput(comStatement) )
	{
		nature = "STIMULATION";
	}
	else
	{
		nature = "OBSERVATION";
	}

	out << EOL << TAB6 << "{"
		<< EOL << TAB7 << "\"name\": \"" << transition.getNameID() << "\","

		<< EOL << TAB7 << "\"guard\": \"" << guard->first().str() << "\","

		<< EOL << TAB7 << "\"guard_smt\": \"" << outSMT.str() << "\","

		<< EOL << TAB7 << "\"action\": {"
		<< EOL << TAB8 << "\"nature\": \"" << nature << "\","

		<< EOL << TAB8 << "\"port\": \"" << port.getNameID() << "\"";
	if( comStatement->getOperands().populated() )
	{
		out << ","
			<< EOL << TAB8 << "\"parameters\": [";
		std::string commaSep = "";
		for( std::size_t offset = 1 ; offset < comStatement->getOperands().size() ; ++offset)
		{
			const BF & param = comStatement->operand(offset);
			out << commaSep
				<< EOL << TAB9 << "\"" << param.to< Variable >().getNameID() << "\"";
			commaSep = ",";
		}
		out << EOL << TAB8 << "]";
	}

	out << EOL << TAB7 << "},";

	const Machine & targetState = transition.getTarget().to< Machine >();
	if( targetState.getSpecifier().isStateMocFINAL() )
	{
		out << EOL << TAB7 << "\"verdict\": \"PASS\"";
	}
	else if( targetState.getSpecifier().isPseudostateMocTERMINAL() )
	{
		out << EOL << TAB7 << "\"verdict\": \""
			<< tcVerdictToJson(targetState.getNameID()) << "\"";
	}
	else
	{
		out << EOL << TAB7 << "\"next\": \"" << targetState.getNameID() << "\"";
	}

	out << EOL << TAB6 << "}";
}


void AvmTestCaseFactory::saveTestCaseJson(const System & aSystemTC)
{
	OutStream & out = mProcessor.newFileStream("testcases.json");
	std::string commaSep = "";

	out << "{"
		<< TC_MANIFEST;

	out << EOL << TAB2 << "\"SUT\": {"
		<< EOL << TAB3 << "\"output_ports\": [";
	for( const auto & itPort : OUTPUT_PORTS )
	{
		out << commaSep;
		sutPortToJson(out, itPort.to< Port >());
		commaSep = ",";
	}
	out << EOL << TAB3 << "],";

	out << EOL << TAB3 << "\"input_ports\": [";
	commaSep = "";
	for( const auto & itPort : INPUT_PORTS )
	{
		out << commaSep;
		sutPortToJson(out, itPort.to< Port >());
		commaSep = ",";
	}
	out << EOL << TAB3 << "],";

	out << EOL << TAB3 << "\"uncontrollable_input_ports\": [";
	commaSep = "";
	for( const auto & itPort : UNCRNTROLLABLE_INPUT_PORTS )
	{
		out << commaSep;
		sutPortToJson(out, itPort.to< Port >());

		commaSep = ",";
	}
	out << EOL << TAB3 << "]"
		<< EOL << TAB2 << "}," << EOL;

	out <<  EOL << TAB2 << "\"TESTCASE\": {"
		<< EOL << TAB3 << "\"variables\": [";

	TableOfVariable::const_raw_iterator itVar = mMachineTC->getVariables().begin();
	TableOfVariable::const_raw_iterator endVar = mMachineTC->getVariables().end();
	commaSep = "";
	for( ; itVar != endVar ; ++itVar )
	{
		out << commaSep;
		out << EOL << TAB4 << "{"
			<< EOL << TAB5 << "\"name\": \"" << itVar->getNameID() << "\","
			<< EOL << TAB5 << "\"type\": \"" << itVar->strTypeSpecifier() << "\""
			<< EOL << TAB4 << "}";
		commaSep = ",";
	}

	out << EOL << TAB3 << "],\n";

	out << EOL << TAB3 << "\"states\": [";
	TableOfMachine::const_raw_iterator itMachine =
			mMachineTC->getCompositePart()->getMachines().begin();
	TableOfMachine::const_raw_iterator endMachine =
			mMachineTC->getCompositePart()->getMachines().end();
	commaSep = "";
	std::string commaSep2;
	for( ; itMachine != endMachine ; ++itMachine )
	{
//		if( itMachine->getSpecifier().isPseudostateMocTERMINAL() // VERDICT
//			|| itMachine->getSpecifier().isStateMocFINAL() )     // STATES
//		{
//			continue;
//		}

		if( itMachine->hasBehaviorPart()
			&& itMachine->getBehaviorPart()->hasOutgoingTransition() )
		{
			std::string name = itMachine->getNameID();
			out << commaSep;
			out << EOL << TAB4 << "{"
				<< EOL << TAB5 << "\"name\": \"" << name << "\",";

			name = name.substr( name.find_first_of('_', 4) + 1 ); // 4 = size("ec_1")
			out << EOL << TAB5 << "\"sut_state\": \"" << name << "\",";

			out << EOL << TAB5 << "\"time_var\": \""
				<< tcTimeVariableToJson(itMachine->to< Machine >()) << "\",";

			out << EOL << TAB5 << "\"transitions\": [";
			commaSep2 = "";
			for( const auto & itTransition :
					itMachine->getBehaviorPart()->getOutgoingTransitions() )
			{
				out << commaSep2;
				tcTransitionToJson(out, itTransition.to< Transition >(), mQuiescencePortTC);
				commaSep2 = ",";
			}
			out << EOL << TAB5 << "]"
				<< EOL << TAB4 << "}";
			commaSep = ",";
		}
	}


	out << EOL << TAB3 << "]\n";

	out << EOL << TAB2 << "}\n";

	out << "}";
}



} /* namespace sep */
