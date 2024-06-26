/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 15 nov. 2012
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#include "CVC4Solver.h"

/*
 * Here because "SolverDef.h" could define this macro
 */
#if defined( _AVM_SOLVER_CVC4_ )


#include <util/avm_vfs.h>

#include <fml/expression/AvmCode.h>
#include <fml/builtin/Boolean.h>
#include <fml/expression/BuiltinContainer.h>
#include <fml/builtin/Character.h>
#include <fml/expression/ExpressionComparer.h>
#include <fml/expression/ExpressionConstant.h>
#include <fml/expression/ExpressionConstructor.h>
#include <fml/expression/ExpressionTypeChecker.h>
#include <fml/numeric/Float.h>
#include <fml/builtin/Identifier.h>
#include <fml/numeric/Integer.h>
#include <fml/numeric/Rational.h>
#include <fml/builtin/String.h>

#include <fml/operator/Operator.h>

#include <fml/runtime/ExecutionContext.h>
#include <fml/runtime/ExecutionData.h>
#include <fml/runtime/RuntimeForm.h>
#include <fml/runtime/RuntimeID.h>
#include <fml/runtime/TableOfData.h>

#include <fml/type/TypeManager.h>

#include <fml/workflow/WObject.h>


namespace sep
{


/*
 *******************************************************************************
 * ID
 *******************************************************************************
 */
std::string CVC4Solver::ID = "CVC4";

std::string CVC4Solver::DESCRIPTION = "CVC4 "
		"'Cooperating Validity Checker 4, Efficient Automatic Theorem Prover "
		"for Satisfiability Modulo Theories problems, BSD License'";

std::uint64_t CVC4Solver::SOLVER_SESSION_ID = 1;


//For automatic destroy of these CVC4 object using assignment
CVC4::Expr CVC4Solver::CVC4_EXPR_NULL;
CVC4::Type CVC4Solver::CVC4_TYPE_NULL;


/**
 * CONSTRUCTOR
 * Default
 */
CVC4Solver::CVC4Solver(bool produce_models_flag)
: base_this_type(CVC4_TYPE_NULL, CVC4_EXPR_NULL),
mParamPrefix( "P_" ),
mExprManager( ),
mSmtEngine( & mExprManager )
{
	mLogFolderLocation = VFS::ProjectDebugPath + "/cvc4/";

	SMT_TYPE_BOOL      = mExprManager.booleanType();

	SMT_TYPE_ENUM      = mExprManager.integerType();

	SMT_TYPE_UINTEGER  = mExprManager.integerType();
	SMT_TYPE_INTEGER   = mExprManager.integerType();

	SMT_TYPE_BV32      = mExprManager.mkBitVectorType(32);
	SMT_TYPE_BV64      = mExprManager.mkBitVectorType(64);

	SMT_TYPE_URATIONAL = mExprManager.realType();
	SMT_TYPE_RATIONAL  = mExprManager.realType();

	SMT_TYPE_UREAL     = mExprManager.realType();
	SMT_TYPE_REAL      = mExprManager.realType();

	SMT_TYPE_NUMBER    = mExprManager.realType();

	SMT_TYPE_STRING    = mExprManager.stringType();

	SMT_CST_BOOL_TRUE  = mExprManager.mkConst(true);
	SMT_CST_BOOL_FALSE = mExprManager.mkConst(false);

	SMT_CST_INT_ZERO   = mExprManager.mkConst(CVC4::Rational(0));
	SMT_CST_INT_ONE    = mExprManager.mkConst(CVC4::Rational(1));

//	// Set the logic : LINEAR ARITHMETIC
//	mSmtEngine.setLogic("QF_LIRA");
//	// Set the logic : BIT VECTOR
//	mSmtEngine.setLogic("QF_BV");

	// Produce Models
	mSmtEngine.setOption("produce-models", produce_models_flag);

	// Enable Multiple Queries
	mSmtEngine.setOption("incremental", true);
	//Disable dagifying the output
	mSmtEngine.setOption("default-dag-thresh", 0);
	// Set the output-language to CVC's
	mSmtEngine.setOption("output-language", "cvc4");
}


/**
 * DESTRUCTOR
 */
CVC4Solver::~CVC4Solver()
{
	SMT_TYPE_BOOL      = CVC4_TYPE_NULL;

	SMT_TYPE_ENUM      = CVC4_TYPE_NULL;

	SMT_TYPE_UINTEGER  = CVC4_TYPE_NULL;
	SMT_TYPE_INTEGER   = CVC4_TYPE_NULL;

	SMT_TYPE_BV32      = CVC4_TYPE_NULL;
	SMT_TYPE_BV64      = CVC4_TYPE_NULL;

	SMT_TYPE_URATIONAL = CVC4_TYPE_NULL;
	SMT_TYPE_RATIONAL  = CVC4_TYPE_NULL;

	SMT_TYPE_UREAL     = CVC4_TYPE_NULL;
	SMT_TYPE_REAL      = CVC4_TYPE_NULL;

	SMT_TYPE_NUMBER    = CVC4_TYPE_NULL;

	SMT_TYPE_STRING    = CVC4_TYPE_NULL;

	SMT_CST_BOOL_TRUE  = CVC4_EXPR_NULL;
	SMT_CST_BOOL_FALSE = CVC4_EXPR_NULL;

	SMT_CST_INT_ZERO   = CVC4_EXPR_NULL;
	SMT_CST_INT_ONE    = CVC4_EXPR_NULL;
}


/**
 * CONFIGURE
 */
bool CVC4Solver::configure(const WObject * wfFilterObject)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )

	std::string logFolderLocation = VFS::ProjectDebugPath + "/cvc4/";

	if ( not VFS::checkWritingFolder(logFolderLocation, true) )
	{
		AVM_OS_LOG << " CVC4Solver::createChecker :> Error: The folder "
				<< "`" << logFolderLocation	<< "' "
				<< "---> doesn't exist or is not writable !!!"
				<< std::endl << std::endl;
		return( false );
	}

AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )

	return( true );
}



/**
 * CREATE - DESTROY
 * ValidityChecker
 */

bool CVC4Solver::createChecker()
{
//	// Set the logic : LINEAR ARITHMETIC
//	mSmtEngine.setLogic("QF_LIRA");
//	// Set the logic : BIT VECTOR
//	mSmtEngine.setLogic("QF_BV");

//	// Produce Models
//	mSmtEngine.setOption("produce-models", false);
//
//	// Enable Multiple Queries
//	mSmtEngine.setOption("incremental", true);
//	//Disable dagifying the output
//	mSmtEngine.setOption("default-dag-thresh", 0);
//	// Set the output-language to CVC's
//	mSmtEngine.setOption("output-language", "cvc4");


AVM_IF_DEBUG_LEVEL_FLAG( HIGH , SMT_SOLVING )

		std::string logFileLocation = ( OSS() << mLogFolderLocation
				<< "log_" << SOLVER_SESSION_ID << ".cvc4" );

//CVC3	mSmtEngine.setOption("dump-log", logFileLocation);
		mSmtEngine.setOption("dump-to", logFileLocation);

		mSmtEngine.setOption("dump", "raw-benchmark");

//		mSmtEngine.setOption("dump", "declarations");
//		mSmtEngine.setOption("dump", "assertions");

		mSmtEngine.setOption("output-lang", "smt2.6");

AVM_ENDIF_DEBUG_LEVEL_FLAG( HIGH , SMT_SOLVING )


//	SMT_TYPE_BOOL      = mExprManager.boolType();
//
//	SMT_TYPE_ENUM      = mExprManager.integerType();
//
//	SMT_TYPE_UINTEGER  = mExprManager.integerType();
//	SMT_TYPE_INTEGER   = mExprManager.integerType();
//
//	SMT_TYPE_BV32      = mExprManager.mkBitVectorType(32);
//	SMT_TYPE_BV64      = mExprManager.mkBitVectorType(64);
//
//	SMT_TYPE_URATIONAL = mExprManager.realType();
//	SMT_TYPE_RATIONAL  = mExprManager.realType();
//
//	SMT_TYPE_UREAL     = mExprManager.realType();
//	SMT_TYPE_REAL      = mExprManager.realType();
//
//	SMT_TYPE_NUMBER    = mExprManager.realType();
//
//	SMT_TYPE_STRING    = mExprManager.stringType();
//
//	SMT_CST_BOOL_TRUE  = mExprManager.mkConst(true);
//	SMT_CST_BOOL_FALSE = mExprManager.mkConst(false);
//
//	SMT_CST_INT_ZERO   = mExprManager.mkConst(0);
//	SMT_CST_INT_ONE    = mExprManager.mkConst(1);

	resetTable();

	return( true );
}

bool CVC4Solver::destroyChecker()
{
	resetTable();

//	SMT_TYPE_BOOL      = CVC4_TYPE_NULL;
//
//	SMT_TYPE_ENUM      = CVC4_TYPE_NULL;
//
//	SMT_TYPE_UINTEGER  = CVC4_TYPE_NULL;
//	SMT_TYPE_INTEGER   = CVC4_TYPE_NULL;
//
//	SMT_TYPE_BV32      = CVC4_TYPE_NULL;
//	SMT_TYPE_BV64      = CVC4_TYPE_NULL;
//
//	SMT_TYPE_URATIONAL = CVC4_TYPE_NULL;
//	SMT_TYPE_RATIONAL  = CVC4_TYPE_NULL;
//
//	SMT_TYPE_UREAL     = CVC4_TYPE_NULL;
//	SMT_TYPE_REAL      = CVC4_TYPE_NULL;
//
//	SMT_TYPE_NUMBER    = CVC4_TYPE_NULL;
//
//	SMT_TYPE_STRING    = CVC4_TYPE_NULL;
//
//	SMT_CST_BOOL_TRUE  = CVC4_EXPR_NULL;
//	SMT_CST_BOOL_FALSE = CVC4_EXPR_NULL;
//
//	SMT_CST_INT_ZERO   = CVC4_EXPR_NULL;
//	SMT_CST_INT_ONE    = CVC4_EXPR_NULL;
//
//	delete( mExprManager );
//	mExprManager = nullptr;

AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )
	++SOLVER_SESSION_ID;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )

	return( true );
}

bool CVC4Solver::resetTable()
{
	base_this_type::resetTable();

	mTableOfParameterExpr.push_back( CVC4_EXPR_NULL );

	mBitsetOfConstrainedParameter.push_back( false );
	mBitsetOfPositiveParameter.push_back( false );
	mBitsetOfStrictlyPositiveParameter.push_back( false );

	return( true );
}



/**
 * PROVER
 */
//bool CVC4Solver::isSubSet(
//		const ExecutionContext & newEC, const ExecutionContext & oldEC)
//{
//	try
//	{
//		createChecker();
//
//		CVC4::Expr newFormula;
//		CVC4::Expr oldFormula;
//
//		// On construit la formule newFormula =
//		// garde_newEC AND
//		// x1 = e1 AND ---- AND xn = en
//		//
//		// et la formule oldFormula =
//		// garde_oldEC AND
//		// x1 = e'1 AND ---- AND xn = e'n AND
//		// e1 = e'1 AND ---- AND en = e'n
//		//
//		edToExprWithVarEquality(newEC.getExecutionData(), newFormula,
//				oldEC.getExecutionData(), oldFormula);
//
//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	oldEC.writeTraceBeforeExec(AVM_OS_TRACE << TAB);
//	AVM_OS_TRACE << TAB << "Expr: " << oldFormula << std::endl;
//
//	newEC.writeTraceBeforeExec(AVM_OS_TRACE << TAB);
//	AVM_OS_TRACE << TAB << "Expr: " << newFormula << std::endl;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )
//
//		// ASSERT de newFormula
//		//
//		mSmtEngine.assertFormula(newFormula);
//
//		// CHECKSAT de oldFormula
//		//
//		// A la place de CHECKSAT on utilise NOT QUERY(NOT oldFormula)
//		//
//		CVC4::Result result = mSmtEngine.query( oldFormula.notExpr() );
//
//		newFormula = CVC4_EXPR_NULL;
//		oldFormula = CVC4_EXPR_NULL;
//		destroyChecker();
//
//		// Si not result alors la formule oldFormula est satisfiable
//		return( result != CVC4::Result::VALID );
//	}
//	catch( const CVC4::Exception & ex )
//	{
//		AVM_OS_TRACE << TAB << "*** Exception caught in CVC4Solver::isSubSet: \n"
//				<< ex << std::endl;
//	}
//
//	return( false );
//}


//bool CVC4Solver::isSubSet(
//		const ExecutionContext & newEC, const ExecutionContext & oldEC)
//{
//	try
//	{
//		createChecker();
//
//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	AVM_OS_TRACE << std::endl << "new EC: " << newEC.getIdNumber() << std::endl
//			<< std::endl << "old EC: " << oldEC.getIdNumber() << std::endl;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )
//
//		// On construit la formule newFormula :
//		// garde_newEC AND
//		// x1 = e1 AND ---- AND xn = en
//		//
//		// et la formule oldFormula :
//		// garde_oldEC AND
//		// x1 = e'1 AND ---- AND xn = e'n AND
//		// e1 = e'1 AND ---- AND en = e'n
//		//
//		CVC4::Expr newFormula;
//		CVC4::Expr oldFormula;
//		edToExprWithVarEquality(newEC.getExecutionData(),
//				newFormula, oldEC.getExecutionData(), oldFormula);
//
//		if( oldFormula != SMT_CST_BOOL_FALSE )
//		{
//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	oldEC.writeTraceBeforeExec(AVM_OS_TRACE << TAB);
//	AVM_OS_TRACE << TAB << "!!!!!!!!!!!!!!! old Expr: " << oldFormula << std::endl;
//
//	newEC.writeTraceBeforeExec(AVM_OS_TRACE << TAB);
//	AVM_OS_TRACE << TAB << "!!!!!!!!!!!!!!! new Expr: " << newFormula << std::endl;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )
//
//			CVC4::Expr implyFormula;
//			if( not mTableOfParameterExprForOldFormula.empty() )
//			{
//				// On construit la formule itexists
//				// mTableOfParameterExprForOldFormula oldFormula
//				CVC4::Expr itExistsFormula = mExprManager.mkExpr(
//						CVC4::kind::EXISTS,
//						mExprManager.mkExpr(CVC4::kind::BOUND_VAR_LIST,
//								mTableOfParameterExprForOldFormula),
//						oldFormula);
//
//				// On construit la formule newFormula => itExistsFormula
//				//
//				implyFormula = mExprManager.mkExpr(
//						CVC4::kind::IMPLIES, newFormula, itExistsFormula);
//			}
//			else
//			{
//				// On construit la formule newFormula => oldFormula
//				//
//				implyFormula = mExprManager.mkExpr(
//						CVC4::kind::IMPLIES, newFormula, oldFormula);
//			}

//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	AVM_OS_TRACE << implyFormula;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )
//
//			// On construit la formule forall
//			// mTableOfParameterExprForNewFormula implyFormula
//			Vector< CVC4::Expr > forAllParameterAndVariableList;
//			forAllParameterAndVariableList.append(mTableOfVariableExpr);
//			forAllParameterAndVariableList.append(
//					mTableOfParameterExprForNewFormula);
//			CVC4::Expr forAllFormula = mExprManager.mkExpr(CVC4::kind::FORALL,
//					mExprManager.mkExpr(CVC4::kind::BOUND_VAR_LIST,
//							forAllParameterAndVariableList),
//					implyFormula);
//			AVM_OS_TRACE << TAB << "--> forAllFormula : ";
//			AVM_OS_TRACE << forAllFormula;
//
//			// QUERY de forAllFormula
//			//
//			CVC4::Result result = mSmtEngine.query( forAllFormula );
//
//			newFormula = CVC4_EXPR_NULL;
//			oldFormula = CVC4_EXPR_NULL;
//			destroyChecker();
//
//			return( result == CVC4::Result::VALID );
//		}
//		else
//		{
//			AVM_OS_TRACE << TAB << "--> cas  oldFormula à false " << std::endl;
//			return( false );
//		}
//	}
//	catch( const CVC4::Exception & ex )
//	{
//		AVM_OS_TRACE << TAB << "*** Exception caught in CVC4Solver::isSubSet: \n"
//				<< ex << std::endl;
//	}
//
//	return( false );
//}



bool CVC4Solver::isSubSet(
		const ExecutionContext & newEC, const ExecutionContext & oldEC)
{
	try
	{
		createChecker();

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << std::endl << "new EC: " << newEC.getIdNumber() << std::endl
			<< std::endl << "old EC: " << oldEC.getIdNumber() << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

		// On construit la formule newFormula :
		// garde_newEC AND
		// x1 = e1 AND ---- AND xn = en
		//
		// la formule oldFormula :
		// garde_oldEC AND
		// x1 = e'1 AND ---- AND xn = e'n
		//
		// et la formule equFormula qui établie l'égalité des mémoires symboliques
		// e1 = e'1 AND ---- AND en = e'n
		//
		CVC4::Expr newFormula;
		CVC4::Expr oldFormula;
		CVC4::Expr equFormula;
		edToExprWithVarEquality(newEC.getExecutionData(), newFormula,
								oldEC.getExecutionData(), oldFormula,
								equFormula);

		bool bResult = false;

		if( oldFormula != SMT_CST_BOOL_FALSE )
		{
AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	oldEC.traceDefault(AVM_OS_TRACE << TAB);
	AVM_OS_TRACE << TAB << "!!!!!!!!!!!!!!! old Expr: " << oldFormula << std::endl;

	newEC.traceDefault(AVM_OS_TRACE << TAB);
	AVM_OS_TRACE << TAB << "!!!!!!!!!!!!!!! new Expr: " << newFormula << std::endl;

//	newEC.writeTraceBeforeExec(AVM_OS_TRACE << TAB);
//	AVM_OS_TRACE << TAB << "!!!!!!!!!!!!!!! equ Expr: " << equFormula << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

			// Déclaration des variables de mTableOfVariableExpr et
			// et des paramètres de mTableOfParameterExpr
			//

			// ASSERT de newFormula
			//
			mSmtEngine.assertFormula(newFormula);

			// ASSERT de equFormula
			//
//			mSmtEngine.assertFormula(equFormula);

			// CHECKSAT de NOT oldFormula
			//
			// A la place on utilise NOT QUERY(oldFormula)
			//



//			CVC4::Result toto = mSmtEngine.query( oldFormula );
			CVC4::Result toto = mSmtEngine.checkSat( oldFormula );

// !!!!!!!!!!!!!!! 271009 : ESSAI AFA temporaire !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//			CVC4::Result toto = mSmtEngine.query(
//					mExprManager.mkExpr(CVC4::kind::NOT, oldFormula) );
//			if(toto == CVC4::SATISFIABLE)
//			{
//				int i = 1;
//			}
//			if(toto == CVC4::INVALID)
//			{
//				int i = 2;
//			}

//			bool result = ( toto == CVC4::Result::VALID );
			bool result = ( toto == CVC4::Result::SAT );

			bResult = ( not result );
		}
		else
		{
AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << TAB << "--> cas  oldFormula à false " << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )
		}

		newFormula = CVC4_EXPR_NULL;
		oldFormula = CVC4_EXPR_NULL;
		equFormula = CVC4_EXPR_NULL;
		destroyChecker();

		return( bResult );
	}
	catch( const CVC4::Exception & ex )
	{
		AVM_OS_TRACE << TAB << "*** Exception caught in CVC4Solver::isSubSet: \n"
				<< ex << std::endl;

		destroyChecker();
	}

	return( false );
}


void CVC4Solver::edToExprWithVarEquality(const ExecutionData & newED,
		CVC4::Expr & newFormula, const ExecutionData & oldED,
		CVC4::Expr & oldFormula, CVC4::Expr & equFormula)
{
//	AVM_OS_ASSERT_FATAL_NULL_POINTER_EXIT( mSmtEngine ) << "ValidityChecker !!!"
//			<< SEND_EXIT;

	std::vector< CVC4::Expr > newFormulaList;
	std::vector< CVC4::Expr > oldFormulaList;
	std::vector< CVC4::Expr > equFormulaList;

	resetTable();

	// compile PCs
	CVC4::Expr aPC;

//	m_pTableOfParameterExpr = &mTableOfParameterExprForNewFormula;
	newFormulaList.push_back( aPC =
			safe_from_baseform(newED.getPathCondition()) );

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << std::endl << "new PC: " << aPC << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

//	m_pTableOfParameterExpr = nullptr;
	oldFormulaList.push_back( aPC =
			safe_from_baseform(oldED.getPathCondition()) );

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << "old PC: " << aPC << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	InstanceOfData * aVar = nullptr;

	// compile DATAs
	std::ostringstream oss;

	avm_offset_t varID = 1;
	for( const auto & itPairMachineData : getSelectedVariable() )
	{
		if( itPairMachineData.second().nonempty() )
		{
			const RuntimeForm & newRF = newED.getRuntime(
					itPairMachineData.first() );
			const RuntimeForm & oldRF = oldED.getRuntime(
					itPairMachineData.first() );

			for( const auto & itVar : itPairMachineData.second() )
			{
				//??? TABLEAUX
				aVar = newRF.rawVariable(itVar->getOffset());
				
				const BaseTypeSpecifier & varTypeSpecifier = aVar->getTypeSpecifier();

				oss.str("");
				oss << "V_" << varID;

//				m_pTableOfParameterExpr = &mTableOfParameterExprForNewFormula;
				CVC4::Expr newValue = safe_from_baseform(newRF.getData(itVar));
//				m_pTableOfParameterExpr = nullptr;
				CVC4::Expr oldValue = safe_from_baseform(oldRF.getData(itVar));

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << varTypeSpecifier.strT() << "  "
			<< itVar->getNameID() << " --> " << oss.str()
			<< std::endl << "\told: " << oldValue << "\tnew: " << newValue;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )


				if( varTypeSpecifier.isTypedBoolean() )
				{
					const BF & newElement = newRF.getData(itVar);
					const BF & oldElement = oldRF.getData(itVar);
					if( newElement.isBoolean() && oldElement.isBoolean() )
					{
						// On rajoute l'équivalence entre la nouvelle valeur
						// symbolique et l'ancienne
//						equFormulaList.push_back( mExprManagermkExpr(
//								CVC4::kind::EQUAL, newValue, oldValue) );
						oldFormulaList.push_back(
								(newElement.toBoolean() == oldElement.toBoolean()) ?
										SMT_CST_BOOL_TRUE : SMT_CST_BOOL_FALSE );

						if( newElement.toBoolean() != oldElement.toBoolean() )
						{
							oldFormula = SMT_CST_BOOL_FALSE;
							resetTable();
							return;
						}
					}
					else
					{
						CVC4::Expr varExpr =
								mExprManager.mkVar(oss.str(), SMT_TYPE_BOOL);
						mTableOfVariableExpr.push_back( varExpr );

						newFormulaList.push_back( mExprManager.mkExpr(
								CVC4::kind::EQUAL, varExpr, newValue) );
						oldFormulaList.push_back( mExprManager.mkExpr(
								CVC4::kind::EQUAL, varExpr, oldValue) );
					}
				}
				else if( varTypeSpecifier.weaklyTypedInteger()
						|| varTypeSpecifier.isTypedEnum()
						|| varTypeSpecifier.isTypedMachine() )
				{
					const BF & newElement = newRF.getData(itVar);
					const BF & oldElement = oldRF.getData(itVar);
					if ( newElement.isNumeric() && oldElement.isNumeric() )
					{
						// On rajoute l'égalité entre la nouvelle valeur
						// symbolique et l'ancienne
//						equFormulaList.push_back( mExprManager.mkExpr(
//								CVC4::kind::EQUAL, newValue, oldValue) );

						oldFormulaList.push_back(
								(ExpressionComparer::isEQ(newElement, oldElement)) ?
								SMT_CST_BOOL_TRUE : SMT_CST_BOOL_FALSE  );

						if( ExpressionComparer::isNEQ(newElement, oldElement) )
						{
							oldFormula = SMT_CST_BOOL_FALSE;
							resetTable();
							return;
						}
					}
					else
					{
						CVC4::Expr varExpr =
								mExprManager.mkVar(oss.str(), SMT_TYPE_INTEGER);
						mTableOfVariableExpr.push_back( varExpr );

						newFormulaList.push_back( mExprManager.mkExpr(
								CVC4::kind::EQUAL, varExpr, newValue) );

						oldFormulaList.push_back( mExprManager.mkExpr(
								CVC4::kind::EQUAL, varExpr, oldValue) );
					}
				}
				else if( varTypeSpecifier.weaklyTypedReal() )
				{
					const BF & newElement = newRF.getData(itVar);
					const BF & oldElement = oldRF.getData(itVar);
					if ( newElement.isNumeric() && oldElement.isNumeric() )
					{
						// On rajoute l'égalité entre la nouvelle valeur
						// symbolique et l'ancienne
//						equFormulaList.push_back( mExprManager.mkExpr(
//								CVC4::kind::EQUAL, newValue, oldValue) );

						oldFormulaList.push_back(
								(ExpressionComparer::isEQ(newElement, oldElement)) ?
								SMT_CST_BOOL_TRUE : SMT_CST_BOOL_FALSE  );

						if( ExpressionComparer::isNEQ(newElement, oldElement) )
						{
							oldFormula = SMT_CST_BOOL_FALSE;
							resetTable();
							return;
						}
					}
					else
					{
						CVC4::Expr varExpr =
								mExprManager.mkVar(oss.str(), SMT_TYPE_REAL);
						mTableOfVariableExpr.push_back( varExpr );

						newFormulaList.push_back( mExprManager.mkExpr(
								CVC4::kind::EQUAL, varExpr, newValue) );

						oldFormulaList.push_back( mExprManager.mkExpr(
								CVC4::kind::EQUAL, varExpr, oldValue) );
					}
				}
				else
				{
					AVM_OS_ERROR_ALERT << "Unexpected an instance type << "
							<< aVar->getFullyQualifiedNameID() << " as : " << oss.str() << " : "
							<< varTypeSpecifier.getFullyQualifiedNameID() << ">> !!!"
							<< SEND_ALERT;

					CVC4::Expr varExpr =
							mExprManager.mkVar(oss.str(), SMT_TYPE_REAL);
					mTableOfVariableExpr.push_back( varExpr );

					newFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, newValue) );

					oldFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, oldValue) );
				}
			}
		}
	}

	for( const auto & itParam : mTableOfParameterExpr )
	{
		if( not mTableOfParameterExprForNewFormula.contains( itParam ) )
		{
			mTableOfParameterExprForOldFormula.append( itParam );
		}
	}

//	oldFormula = mSmtEngine.simplify(
//			mExprManager.mkExpr(CVC4::kind::AND, oldFormulaList) );
//
//	newFormula = mSmtEngine.simplify(
//			mExprManager.mkExpr(CVC4::kind::AND, newFormulaList) );

// Temporaire pour trace : remplacer ensuite par les 2 égalités précédentes
	CVC4::Expr oldFormulAvantSimplfy =
			mExprManager.mkExpr(CVC4::kind::AND, oldFormulaList);

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << std::endl << "oldFormula avant simplify : "
			<< oldFormulAvantSimplfy << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	oldFormula = mSmtEngine.simplify( oldFormulAvantSimplfy );

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << "oldFormula après simplify : "
			<< oldFormula << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )


	CVC4::Expr newFormulAvantSimplfy =
			mExprManager.mkExpr(CVC4::kind::AND, newFormulaList);

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << std::endl << "newFormula avant simplify : "
			<< newFormulAvantSimplfy << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	newFormula = mSmtEngine.simplify( newFormulAvantSimplfy );

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << "newFormula après simplify : "
			<< newFormula << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

//	CVC4::Expr equFormulAvantSimplfy = mExprManager.mkExpr(CVC4::kind::AND, equFormulaList);
//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	AVM_OS_TRACE << std::endl << "equFormula avant simplify : ";
//			<< equFormulAvantSimplfy << std::endl;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )
//	equFormula = mSmtEngine.simplify( equFormulAvantSimplfy );

	resetTable();
}



bool CVC4Solver::isEqualSet(
		const ExecutionContext & newEC, const ExecutionContext & oldEC)
{
	try
	{
		createChecker();

		CVC4::Expr newFormula;
		CVC4::Expr oldFormula;

		edToExpr(newEC.getExecutionData(), newFormula,
				oldEC.getExecutionData(), oldFormula);

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	oldEC.traceDefault(AVM_OS_TRACE << TAB);
	AVM_OS_TRACE << TAB << "Expr: " << oldFormula << std::endl;

	newEC.traceDefault(AVM_OS_TRACE << TAB);
	AVM_OS_TRACE << TAB << "Expr: " << newFormula << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

//		CVC4::Expr equalTest = mExprManager.mkExpr(CVC4::kind::FORALL,
//			mExprManager.mkExpr(CVC4::kind::BOUND_VAR_LIST, mTableOfVariableExpr),
//			mExprManager.mkExpr(CVC4::kind::EXISTS,
//				mExprManager.mkExpr(CVC4::kind::BOUND_VAR_LIST, mTableOfParameterExpr),
//				mExprManager.mkExpr(CVC4::kind::EQUAL, oldFormula, newFormula)));
//
//		CVC4::Expr equalTest = mExprManager.mkExpr(CVC4::kind::EXISTS,
//				mTableOfParameterExpr,
//				mExprManager.mkExpr(CVC4::kind::EQUAL,
//						oldFormula, newFormula));

		CVC4::Expr equalTest = mExprManager.mkExpr(CVC4::kind::EQUAL,
				oldFormula, newFormula);

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << TAB << "isSubSet Expr: " << equalTest << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

//		CVC4::Result result = mSmtEngine.query(equalTest);
		CVC4::Result result = mSmtEngine.checkSat(equalTest);

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << TAB << "\nisEqual result : "
//			<< TAB << ( (result.isValid() == CVC4::Result::VALID) ?
			<< TAB << ( (result.isSat() == CVC4::Result::SAT) ?
					"true" : "false" ) << std::endl	<< std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

		newFormula = CVC4_EXPR_NULL;
		oldFormula = CVC4_EXPR_NULL;
		destroyChecker();

//		return( result == CVC4::Result::VALID );
		return( result == CVC4::Result::SAT );
	}
	catch( const CVC4::Exception & ex )
	{
		AVM_OS_TRACE << TAB << "*** Exception caught in CVC4Solver::isEqualSet: \n"
				<< ex << std::endl;

		destroyChecker();
	}

	return( false );
}


SolverDef::SATISFIABILITY_RING CVC4Solver::isSatisfiable(const BF & aCondition)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )
	AVM_OS_TRACE << "CVC4Solver::isSatisfiable(...) "
			":" << SOLVER_SESSION_ID << ">" << std::endl
			<< "\t" << aCondition.str() << std::endl;

	// trace to file
//	smt_check_sat(aCondition);
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )

	if( aCondition.isBoolean() )
	{
AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << TAB << "is satisfiable : " << aCondition.str() << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

		return( aCondition.toBoolean() ?
				SolverDef::SATISFIABLE : SolverDef::UNSATISFIABLE );
	}

	else if( aCondition.isFloat() )
	{
AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << TAB << "is satisfiable : " << aCondition.str() << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

		return( ( aCondition.toFloat() != 0 ) ?
				SolverDef::SATISFIABLE : SolverDef::UNSATISFIABLE );
	}

	try
	{
		createChecker();

		// compile Formula
		CVC4::Expr aFormula = safe_from_baseform( aCondition );

AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )
	AVM_OS_TRACE << "CVC4Condition :" << SOLVER_SESSION_ID << ">" << std::endl;

	dbg_smt(aFormula);

	AVM_OS_TRACE << "\t" << aFormula << std::endl;

	if( mBitsetOfConstrainedParameter.anyTrue() )
	{
		AVM_OS_TRACE << "REQUIRED ASSERTION : " << mBitsetOfConstrainedParameter
				<< " for CONSTRAINED type" << std::endl;
	}
	if( mBitsetOfPositiveParameter.anyTrue() )
	{
		AVM_OS_TRACE << "REQUIRED ASSERTION : " << mBitsetOfPositiveParameter
				<< " for POSITIVE type" << std::endl;
	}
	if( mBitsetOfStrictlyPositiveParameter.anyTrue() )
	{
		AVM_OS_TRACE << "REQUIRED ASSERTION : "
				<< mBitsetOfStrictlyPositiveParameter
				<< " for STRICTLY POSITIVE type" << std::endl;
	}
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )

//AVM_IF_DEBUG_LEVEL_FLAG( HIGH , SMT_SOLVING )	// trace to file
//	AVM_OS_TRACE << "z3::solver::to_smt2(...)" << std::endl
//			<< z3Solver.to_smt2() << std::endl;
//AVM_ENDIF_DEBUG_LEVEL_FLAG( HIGH , SMT_SOLVING )


		CVC4::Result result = ( mBitsetOfConstrainedParameter.allFalse()
				|| appendPossitiveAssertion() )
						? mSmtEngine.checkSat( aFormula )
						: CVC4::Result::SAT_UNKNOWN;

		// ATTENTION : lorsque CVC4 répond UNKNOWN,
		// on considère que la garde est SATISFIABLE
		SolverDef::SATISFIABILITY_RING satisfiability = SolverDef::SATISFIABLE;

		switch( result.isSat() )
		{
			case CVC4::Result::SAT:
			//case CVC4::VALID:
			{
				satisfiability = SolverDef::SATISFIABLE;
				break;
			}

			case CVC4::Result::UNSAT:
			//case CVC4::INVALID:
			{
				satisfiability = SolverDef::UNSATISFIABLE;
				break;
			}

			case CVC4::Result::SAT_UNKNOWN:
			{
				satisfiability = SolverDef::UNKNOWN_SAT;
				break;
			}

//			case CVC4::Result::ABORT:
			default:
			{
				satisfiability = SolverDef::ABORT_SAT;
				break;
			}
		}

AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )
	AVM_OS_TRACE << "result :" << SOLVER_SESSION_ID << "> "
			<< SolverDef::strSatisfiability(satisfiability) << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )

		aFormula = CVC4_EXPR_NULL;
		destroyChecker();

		return( satisfiability );
	}
	catch ( const CVC4::Exception & ex )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::isSatisfiable< CVC4::Exception :"
				<< SOLVER_SESSION_ID << ">\n\t" << ex << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\t  " << aCondition.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( SolverDef::UNKNOWN_SAT );
	}
	catch ( const std::exception & ex )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::isSatisfiable< std::exception :"
				<< SOLVER_SESSION_ID << ">\n\t" << ex.what() << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\t  " << aCondition.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( SolverDef::UNKNOWN_SAT );
	}
	catch ( ... )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::isSatisfiable< unknown::exception :"
				<< SOLVER_SESSION_ID << ">" << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\t  " << aCondition.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( SolverDef::UNKNOWN_SAT );
	}

	return( SolverDef::UNKNOWN_SAT );
}



/**
 * SOLVER
 */
bool CVC4Solver::solveImpl(const BF & aCondition,
		BFVector & dataVector, BFVector & valuesVector)
{
AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )
	AVM_OS_TRACE << "CVC4Solver::solve(...) "
			":" << SOLVER_SESSION_ID << ">" << std::endl
			<< "\t" << aCondition.str() << std::endl;

	// trace to file
//	smt_check_sat(aCondition);
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )

	try
	{
		// Produce Models option --> true
		createChecker( );

		resetTable();

		// compile Formula
		CVC4::Expr aFormula = safe_from_baseform( aCondition );


AVM_IF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )
	AVM_OS_TRACE << "CVC4Condition :" << SOLVER_SESSION_ID << ">" << std::endl;

	dbg_smt(aFormula, true);

	AVM_OS_TRACE << "\t" << aFormula << std::endl;
AVM_ENDIF_DEBUG_LEVEL_FLAG( MEDIUM , SMT_SOLVING )


		CVC4::Result result = mSmtEngine.checkSat( aFormula );

		if( result.isSat() == CVC4::Result::SAT )
		{
//			CVC4::Model * concreteModel = mSmtEngine.getModel();

			for( std::size_t i = 1 ; i < mTableOfParameterInstance.size() ; ++i )
			{
				dataVector.append( mTableOfParameterInstance[i] );

				const CVC4::Expr & var = mTableOfParameterExpr[i];
				CVC4::Expr val = mSmtEngine.getValue(var);

				if( val == SMT_CST_BOOL_TRUE )
				{
					valuesVector.append( ExpressionConstant::BOOLEAN_TRUE );
				}
				else if( val == SMT_CST_BOOL_FALSE )
				{
					valuesVector.append( ExpressionConstant::BOOLEAN_FALSE );
				}
				else if( val.getKind() == CVC4::kind::CONST_RATIONAL )
				{
					CVC4::Rational rat = val.getConst< CVC4::Rational >();

#if defined( _AVM_BUILTIN_NUMERIC_GMP_ )

					if( rat.isIntegral() )
					{
						valuesVector.append( ExpressionConstructor::newInteger(
								rat.getNumerator().getValue() ) );
					}
					else // C'est un rationel (num/den)
					{
						valuesVector.append(
								ExpressionConstructor::newRational(
										rat.getValue() ) );
					}

#else

					if( rat.isIntegral() )
					{
						if( rat.sgn() > 0 )
						{
							valuesVector.append( ExpressionConstructor::newUInteger(
									rat.getNumerator().getUnsignedLong()) );
						}
						else
						{
							valuesVector.append( ExpressionConstructor::newInteger(
									rat.getNumerator().getLong()) );
						}
					}
					else // C'est un rationel (num/den)
					{
						valuesVector.append(
								ExpressionConstructor::newRational(
									static_cast< avm_integer_t >(
										rat.getNumerator().getLong() ),
									static_cast< avm_integer_t >(
										rat.getDenominator().getLong() ) ) );
					}

#endif /* _AVM_BUILTIN_NUMERIC_GMP_ */

				}
				else
				{
					valuesVector.append( dataVector.back() );

				}

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << mTableOfParameterInstance.rawAt( i )->getFullyQualifiedNameID()
			<< " --> " << var << " = " << val << std::endl;
//	AVM_OS_COUT<< mTableOfParameterInstance.rawAt( i )->getFullyQualifiedNameID()
//			<< " --> " << var << " = " << val << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )
			}
		}

		aFormula = CVC4_EXPR_NULL;
		destroyChecker();

		return( result.isSat() == CVC4::Result::SAT );
	}
	catch ( const CVC4::Exception & ex )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::solve< CVC4::Exception :"
				<< SOLVER_SESSION_ID << ">\n\t" << ex << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\t  " << aCondition.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( false );
	}
	catch ( const std::exception & ex )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::solve< std::exception :"
				<< SOLVER_SESSION_ID << ">\n\t" << ex.what() << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\t  " << aCondition.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( false );
	}
	catch ( ... )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::solve< unknown::exception :"
				<< SOLVER_SESSION_ID << ">" << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\t  " << aCondition.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( false );
	}

	return( false );
}


/**
 * TOOLS
 */

void CVC4Solver::edToExpr(
		const ExecutionData & newED, CVC4::Expr & newFormula,
		const ExecutionData & oldED, CVC4::Expr & oldFormula)
{
//	AVM_OS_ASSERT_FATAL_NULL_POINTER_EXIT( mSmtEngine ) << "ValidityChecker !!!"
//			<< SEND_EXIT;

	resetTable();

	// compile PCs
	CVC4::Expr aPC;
	std::vector< CVC4::Expr > newFormulaList;
	newFormulaList.push_back( aPC = safe_from_baseform(newED.getPathCondition()) );

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << std::endl << "new PC: " << aPC << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	std::vector< CVC4::Expr > oldFormulaList;
	oldFormulaList.push_back( aPC = safe_from_baseform(oldED.getPathCondition()) );

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << "old PC: " << aPC << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	InstanceOfData * aVar = nullptr;

	// compile DATAs
	std::ostringstream oss;

	avm_offset_t varID = 1;
	for( const auto & itPairMachineData : getSelectedVariable() )
	{
		if( itPairMachineData.second().nonempty() )
		{
			const RuntimeForm & newRF = newED.getRuntime(
					itPairMachineData.first() );
			const RuntimeForm & oldRF = oldED.getRuntime(
					itPairMachineData.first() );

			for( const auto & itVar : itPairMachineData.second() )
			{
				//??? TABLEAUX
				aVar = newRF.rawVariable(itVar->getOffset());
				
				const BaseTypeSpecifier & varTypeSpecifier = aVar->getTypeSpecifier();

				oss.str("");
				oss << "V_" << varID;

				CVC4::Expr newValue = safe_from_baseform(newRF.getData(itVar));
				CVC4::Expr oldValue = safe_from_baseform(oldRF.getData(itVar));

				if( varTypeSpecifier.isTypedBoolean() )
				{
					CVC4::Expr varExpr =
							mExprManager.mkVar(oss.str(), SMT_TYPE_BOOL);
					mTableOfVariableExpr.push_back( varExpr );
					newFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, newValue) );
					oldFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL,varExpr, oldValue) );
				}
				else if( varTypeSpecifier.isTypedEnum() )
				{
					CVC4::Expr varExpr =
							mExprManager.mkVar(oss.str(), SMT_TYPE_ENUM);
					mTableOfVariableExpr.push_back( varExpr );
					newFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, newValue) );
					oldFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, oldValue) );
				}
				else if( varTypeSpecifier.weaklyTypedInteger() )
				{
					CVC4::Expr varExpr =
							mExprManager.mkVar(oss.str(), SMT_TYPE_INTEGER);
					mTableOfVariableExpr.push_back( varExpr );
					newFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, newValue) );
					oldFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, oldValue) );
				}
				else if( varTypeSpecifier.weaklyTypedReal() )
				{
					CVC4::Expr varExpr =
							mExprManager.mkVar(oss.str(), SMT_TYPE_REAL);
					mTableOfVariableExpr.push_back( varExpr );
					newFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, newValue) );
					oldFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, oldValue) );
				}
				else
				{
					AVM_OS_ERROR_ALERT << "Unexpected an instance type << "
							<< aVar->getFullyQualifiedNameID() << " as : " << oss.str() << " : "
							<< varTypeSpecifier.getFullyQualifiedNameID() << ">> !!!"
							<< SEND_ALERT;

					CVC4::Expr varExpr = mExprManager.mkVar(oss.str(), SMT_TYPE_REAL);
					mTableOfVariableExpr.push_back( varExpr );
					newFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, newValue) );
					oldFormulaList.push_back( mExprManager.mkExpr(
							CVC4::kind::EQUAL, varExpr, oldValue) );
				}
			}
		}
	}

	oldFormula = mSmtEngine.simplify(
			mExprManager.mkExpr(CVC4::kind::AND, oldFormulaList) );

	newFormula = mSmtEngine.simplify(
			mExprManager.mkExpr(CVC4::kind::AND, newFormulaList) );


	resetTable();
}


/*
CVC4::Expr CVC4Solver::edToExpr(const ExecutionData & anED)
{
	AVM_OS_ASSERT_FATAL_NULL_POINTER_EXIT( mSmtEngine ) << "ValidityChecker !!!"
			<< SEND_EXIT;

//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	AVM_OS_TRACE << TAB << "edToExpr ED: \n";
//	anED.toStreamData(AVM_OS_TRACE, "\t");
//	AVM_OS_TRACE << std::flush;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	resetTable();

	// compile PC
	CVC4::Expr pcExpr = mSmtEngine.simplify(safe_from_baseform(anED.getPathCondition()));
AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << TAB << "\nPC: " << pcExpr << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )
	mSmtEngine.assertFormula( pcExpr );

	std::vector< CVC4::Expr > aFormulaList;
	InstanceOfData * aVar = nullptr;
	ListOfInstanceOfData::iterator itVar;

	// compile DATA
	for( const auto & itPairMachineData : getSelectedVariable() )
	{
		if( itPairMachineData.second().nonempty() )
		{
			itVar = itPairMachineData.begin();
			RuntimeForm * aRF = anED.get(itVar);

			for( ; itVar != itPairMachineData.end() ; ++itVar )
			{
				aVar = aRF.refExecutable().getBasicVariable(itVar->getOffset());
				
				const BaseTypeSpecifier & varTypeSpecifier = aVar->getTypeSpecifier();

				if( varTypeSpecifier.isTypedBoolean() )
				{
					aFormulaList.push_back(
							safe_from_baseform(aRF.getData(itVar)) );
				}
				else if( varTypeSpecifier.weaklyTypedInteger()
						|| varTypeSpecifier.isTypedEnum() )
				{
					aFormulaList.push_back(
							safe_from_baseform(aRF.getData(itVar)) );
				}
				else if( varTypeSpecifier.weaklyTypedReal() )
				{
					aFormulaList.push_back(
							safe_from_baseform(aRF.getData(itVar)) );
				}
				else
				{
					AVM_OS_ERROR_ALERT << "Unexpected an instance type << "
							<< aVar->getFullyQualifiedNameID() " : "
							<< varTypeSpecifier.getFullyQualifiedNameID() << ">> !!!"
							<< SEND_ALERT;

					aFormulaList.push_back(
							safe_from_baseform(aRF.getData(itVar)) );
				}
			}
		}
	}

	CVC4::Expr edExpr =  mExprManager.mkExpr(CVC4::kind::TUPLE, aFormulaList);

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << TAB << "Expr: ";
	AVM_OS_TRACE << edExpr << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	resetTable();

	return( edExpr );
}

*/


CVC4::Expr CVC4Solver::edToExpr(const ExecutionData & anED)
{
//	AVM_OS_ASSERT_FATAL_NULL_POINTER_EXIT( mSmtEngine ) << "ValidityChecker !!!"
//			<< SEND_EXIT;

//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	AVM_OS_TRACE << TAB << "edToExpr ED: \n";
//	anED.toStreamData(AVM_OS_TRACE, "\t");
//	AVM_OS_TRACE << std::flush;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	resetTable();

	std::vector< CVC4::Expr > aFormulaList;

	// compile PC
	aFormulaList.push_back( safe_from_baseform(anED.getPathCondition()) );

	// compile DATA
	avm_offset_t varID = 0;
	for( const auto & itPairMachineData : getSelectedVariable() )
	{
		if( itPairMachineData.second().nonempty() )
		{
			const RuntimeForm & aRF = anED.getRuntime(
					itPairMachineData.first() );

			for( const auto & itVar : itPairMachineData.second() )
			{
				//??? TABLEAUX
				InstanceOfData * aVar = aRF.rawVariable(itVar->getOffset());

				const BaseTypeSpecifier & varTypeSpecifier = aVar->getTypeSpecifier();

				if( varTypeSpecifier.isTypedBoolean() )
				{
					aFormulaList.push_back( mExprManager.mkExpr(CVC4::kind::EQUAL,
							getVariableExpr(aVar, SMT_TYPE_BOOL, varID),
							safe_from_baseform(aRF.getData(itVar))) );
				}
				else if( varTypeSpecifier.isTypedEnum() )
				{
					aFormulaList.push_back( mExprManager.mkExpr(CVC4::kind::EQUAL,
							getVariableExpr(aVar, SMT_TYPE_ENUM, varID),
							safe_from_baseform(aRF.getData(itVar))) );
				}
				else if( varTypeSpecifier.weaklyTypedInteger() )
				{
					aFormulaList.push_back( mExprManager.mkExpr(CVC4::kind::EQUAL,
							getVariableExpr(aVar, SMT_TYPE_INTEGER, varID),
							safe_from_baseform(aRF.getData(itVar))) );
				}
				else if( varTypeSpecifier.weaklyTypedReal() )
				{
					aFormulaList.push_back( mExprManager.mkExpr(CVC4::kind::EQUAL,
							getVariableExpr(aVar, SMT_TYPE_REAL, varID),
							safe_from_baseform(aRF.getData(itVar))) );
				}
				else
				{
					AVM_OS_ERROR_ALERT << "Unexpected an instance type << "
							<< aVar->getFullyQualifiedNameID() << " : "
							<< varTypeSpecifier.getFullyQualifiedNameID() << ">> !!!"
							<< SEND_ALERT;

					aFormulaList.push_back( mExprManager.mkExpr(CVC4::kind::EQUAL,
							getVariableExpr(aVar, SMT_TYPE_REAL, varID),
							safe_from_baseform(aRF.getData(itVar))) );
				}
			}
		}
	}

	CVC4::Expr edExpr = mExprManager.mkExpr(CVC4::kind::EXISTS,
			mExprManager.mkExpr(
					CVC4::kind::BOUND_VAR_LIST, mTableOfParameterExpr),
			mSmtEngine.simplify(
					mExprManager.mkExpr(CVC4::kind::AND, aFormulaList)) );

AVM_IF_DEBUG_FLAG( SMT_SOLVING )
	AVM_OS_TRACE << TAB << "Expr: ";
	AVM_OS_TRACE << edExpr << std::endl;
AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

	resetTable();

	return( edExpr );
}


CVC4::Expr & CVC4Solver::getParameterExpr(const BF & bfParameter)
{
	InstanceOfData & aParameter = const_cast< InstanceOfData & >(
			bfParameter.to< InstanceOfData >() );

	if( aParameter.getMark() == 0 )
	{
		const BaseTypeSpecifier & paramTypeSpecifier =
				aParameter.referedTypeSpecifier();

		CVC4::Type paramType;

		if( paramTypeSpecifier.isTypedBoolean() )
		{
			paramType = SMT_TYPE_BOOL;
		}
		else if( paramTypeSpecifier.weaklyTypedUInteger() )
		{
			if( SolverDef::DEFAULT_SOLVER_KIND ==
					SolverDef::SOLVER_CVC4_BV32_KIND )
			{
				paramType = SMT_TYPE_BV32;
			}
			else
//			if( SolverDef::DEFAULT_SOLVER_KIND == SolverDef::SOLVER_CVC4_KIND )
			{
				paramType = SMT_TYPE_UINTEGER;
			}
		}
		else if( paramTypeSpecifier.weaklyTypedInteger() )
		{
			if( SolverDef::DEFAULT_SOLVER_KIND ==
					SolverDef::SOLVER_CVC4_BV32_KIND )
			{
				paramType = SMT_TYPE_BV32;
			}
			else
//			if( SolverDef::DEFAULT_SOLVER_KIND == SolverDef::SOLVER_CVC4_KIND )
			{
				paramType = SMT_TYPE_INTEGER;
			}
//			else
//			{
//				AVM_OS_FATAL_ERROR_EXIT
//						<< "SolverDef::DEFAULT_SOLVER_KIND <> "
//							"CVC4_INT and <> CVC4_BV32 !!!\n"
//						<< SEND_EXIT;
//
//				paramType = SMT_TYPE_INTEGER;
//			}
		}

		else if( paramTypeSpecifier.weaklyTypedURational() )
		{
			paramType = SMT_TYPE_URATIONAL;
		}
		else if( paramTypeSpecifier.weaklyTypedRational() )
		{
			paramType = SMT_TYPE_RATIONAL;
		}

		else if( paramTypeSpecifier.weaklyTypedUReal() )
		{
			paramType = SMT_TYPE_UREAL;
		}
		else if( paramTypeSpecifier.weaklyTypedReal() )
		{
			paramType = SMT_TYPE_REAL;
		}

		else if( paramTypeSpecifier.isTypedString() )
		{
			paramType = SMT_TYPE_STRING;
		}
		else if( paramTypeSpecifier.isTypedEnum() )
		{
			paramType = SMT_TYPE_ENUM;
			// TODO Attention : il faudrait rajouter les contraintes
			// d'intervalle pour le type énuméré
		}
		else if( paramTypeSpecifier.isTypedMachine() )
		{
			// TODO:> Consolidation après TEST
			paramType = SMT_TYPE_INTEGER;
		}
		else
		{
			AVM_OS_ERROR_ALERT << "Unexpected an instance type << "
					<< aParameter.getFullyQualifiedNameID() << " : "
					<< paramTypeSpecifier.getFullyQualifiedNameID() << ">> !!!"
					<< SEND_ALERT;

			paramType = SMT_TYPE_REAL;
		}


		CVC4::Expr paramExpr = mExprManager.mkVar(
				uniqParameterID( aParameter ), paramType );

//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	AVM_OS_TRACE << TAB  << mParamPrefix << mTableOfParameterInstance.size()
//			<< " <- " << aParameter.getFullyQualifiedNameID() << std::endl;
//	AVM_OS_TRACE << std::flush;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

		aParameter.setMark( mTableOfParameterInstance.size() );

		mTableOfParameterInstance.push_back( bfParameter );

		mTableOfParameterExpr.push_back( paramExpr );

		mBitsetOfConstrainedParameter.push_back(
				paramTypeSpecifier.couldGenerateConstraint() );

		mBitsetOfPositiveParameter.push_back(
				paramTypeSpecifier.isTypedPositiveNumber() );

		mBitsetOfStrictlyPositiveParameter.push_back(
				paramTypeSpecifier.isTypedStrictlyPositiveNumber() );

//		if( m_pTableOfParameterExpr != nullptr )
//		{
//			m_pTableOfParameterExpr->append(paramExpr);
//		}
	}

	return( mTableOfParameterExpr[ aParameter.getMark() ] );
}



CVC4::Expr & CVC4Solver::getBoundParameterExpr(
		const BF & bfParameter, ARGS & boundVarConstraints)
{
	InstanceOfData & aBoundParameter = const_cast< InstanceOfData & >(
			bfParameter.to< InstanceOfData >() );

//	if( aBoundParameter.getMark() == 0 )
	{
		const BaseTypeSpecifier & paramTypeSpecifier =
				aBoundParameter.referedTypeSpecifier();

		CVC4::Type paramType;

		if( paramTypeSpecifier.isTypedBoolean() )
		{
			paramType = SMT_TYPE_BOOL;
		}
		else if( paramTypeSpecifier.weaklyTypedUInteger() )
		{
			if( SolverDef::DEFAULT_SOLVER_KIND ==
					SolverDef::SOLVER_CVC4_BV32_KIND )
			{
				paramType = SMT_TYPE_BV32;
			}
			else
//			if( SolverDef::DEFAULT_SOLVER_KIND == SolverDef::SOLVER_CVC4_KIND )
			{
				paramType = SMT_TYPE_UINTEGER;
			}
		}
		else if( paramTypeSpecifier.weaklyTypedInteger() )
		{
			if( SolverDef::DEFAULT_SOLVER_KIND ==
					SolverDef::SOLVER_CVC4_BV32_KIND )
			{
				paramType = SMT_TYPE_BV32;
			}
			else
//			if( SolverDef::DEFAULT_SOLVER_KIND == SolverDef::SOLVER_CVC4_KIND )
			{
				paramType = SMT_TYPE_INTEGER;
			}
//			else
//			{
//				AVM_OS_FATAL_ERROR_EXIT
//						<< "SolverDef::DEFAULT_SOLVER_KIND <> "
//							"CVC4_INT and <> CVC4_BV32 !!!\n"
//						<< SEND_EXIT;
//
//				paramType = SMT_TYPE_INTEGER;
//			}
		}

		else if( paramTypeSpecifier.weaklyTypedURational() )
		{
			paramType = SMT_TYPE_URATIONAL;
		}
		else if( paramTypeSpecifier.weaklyTypedRational() )
		{
			paramType = SMT_TYPE_RATIONAL;
		}

		else if( paramTypeSpecifier.weaklyTypedUReal() )
		{
			paramType = SMT_TYPE_UREAL;
		}
		else if( paramTypeSpecifier.weaklyTypedReal() )
		{
			paramType = SMT_TYPE_REAL;
		}

		else if( paramTypeSpecifier.isTypedString() )
		{
			paramType = SMT_TYPE_STRING;
		}
		else if( paramTypeSpecifier.isTypedEnum() )
		{
			paramType = SMT_TYPE_ENUM;
			// TODO Attention : il faudrait rajouter les contraintes
			// d'intervalle pour le type énuméré
		}
		else if( paramTypeSpecifier.isTypedMachine() )
		{
			// TODO:> Consolidation après TEST
			paramType = SMT_TYPE_INTEGER;
		}
		else
		{
			AVM_OS_ERROR_ALERT << "Unexpected an instance type << "
					<< aBoundParameter.getFullyQualifiedNameID() << " : "
					<< paramTypeSpecifier.getFullyQualifiedNameID() << ">> !!!"
					<< SEND_ALERT;

			paramType = SMT_TYPE_REAL;
		}


		CVC4::Expr paramExpr = mExprManager.mkBoundVar(
//				OSS() << mParamPrefix << mTableOfParameterInstance.size(),
				aBoundParameter.getNameID(),
				paramType );

//AVM_IF_DEBUG_FLAG( SMT_SOLVING )
//	AVM_OS_TRACE << TAB  << mParamPrefix << mTableOfParameterInstance.size()
//			<< " <- " << aBoundParameter.getFullyQualifiedNameID() << std::endl;
//	AVM_OS_TRACE << std::flush;
//AVM_ENDIF_DEBUG_FLAG( SMT_SOLVING )

		aBoundParameter.setMark( mTableOfParameterInstance.size() );

		mTableOfParameterInstance.push_back( bfParameter );

		mTableOfParameterExpr.push_back( paramExpr );

		mBitsetOfStrictlyPositiveParameter.push_back( false );
		mBitsetOfPositiveParameter.push_back( false );
		mBitsetOfConstrainedParameter.push_back( false );

		if( paramTypeSpecifier.isTypedStrictlyPositiveNumber() )
		{
			boundVarConstraints.next( mExprManager.mkExpr(
					CVC4::kind::GT, paramExpr, SMT_CST_INT_ZERO) );
		}
		else if( paramTypeSpecifier.isTypedPositiveNumber() )
		{
			boundVarConstraints.next( mExprManager.mkExpr(
					CVC4::kind::GEQ, paramExpr, SMT_CST_INT_ZERO) );
		}
		else
		{
			boundVarConstraints.next( SMT_CST_BOOL_TRUE );
		}

//		if( m_pTableOfParameterExpr != nullptr )
//		{
//			m_pTableOfParameterExpr->append(paramExpr);
//		}
	}

	return( mTableOfParameterExpr[ aBoundParameter.getMark() ] );
}



CVC4::Expr & CVC4Solver::getVariableExpr(InstanceOfData * aVar,
		CVC4::Type varType, avm_offset_t varID)
{
	if( mTableOfVariableExpr.size() <= varID )
	{
		mTableOfVariableExpr.push_back(
				mExprManager.mkVar( uniqVariableID( *aVar, varID ), varType ) );
	}

	return( mTableOfVariableExpr.at(varID) );
}



bool CVC4Solver::appendPossitiveAssertion()
{
	std::size_t endOffset = mBitsetOfConstrainedParameter.size();
	for( std::size_t offset = 1 ; offset < endOffset ; ++offset )
	{
		if( mBitsetOfStrictlyPositiveParameter[offset] )
		{
			if( mSmtEngine.assertFormula( mExprManager.mkExpr(CVC4::kind::GT,
					mTableOfParameterExpr[offset], SMT_CST_INT_ZERO) ).isSat()
				== CVC4::Result::UNSAT )
			{
				return( false );
			}
		}
		else if( mBitsetOfPositiveParameter[offset] )
		{
			if( mSmtEngine.assertFormula( mExprManager.mkExpr(CVC4::kind::GEQ,
					mTableOfParameterExpr[offset], SMT_CST_INT_ZERO) ).isSat()
				== CVC4::Result::UNSAT )
			{
				return( false );
			}
		}
	}

	return( true );
}


CVC4::Expr CVC4Solver::safe_from_baseform(const BF & exprForm)
{
	try
	{
		return( from_baseform(exprForm) );
	}
	catch ( const CVC4::Exception & ex )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::safe_from_baseform< CVC4::Exception :"
				<< SOLVER_SESSION_ID << ">\n\t" << ex << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\tFailed to CONVERT sep::fml::expression to CVC4::Expr:>" << std::endl
				<< "\t  " << exprForm.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( SMT_CST_INT_ZERO );
	}
	catch ( const std::exception & ex )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::safe_from_baseform< std::exception :"
				<< SOLVER_SESSION_ID << ">\n\t" << ex.what() << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\tFailed to CONVERT sep::fml::expression to CVC4::Expr:>" << std::endl
				<< "\t  " << exprForm.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( SMT_CST_INT_ZERO );
	}
	catch ( ... )
	{
		AVM_OS_WARN << std::endl << REPEAT("********", 10) << std::endl
				<< "\tCVC4Solver::safe_from_baseform< unknown::exception :"
				<< SOLVER_SESSION_ID << ">" << std::endl
				<< REPEAT("--------", 10) << std::endl
				<< "\tFailed to CONVERT sep::fml::expression to CVC4::Expr:>" << std::endl
				<< "\t  " << exprForm.str() << std::endl
				<< REPEAT("********", 10) << std::endl;

		destroyChecker();

		return( SMT_CST_INT_ZERO );
	}

	return( SMT_CST_INT_ZERO );
}

CVC4::Expr CVC4Solver::from_baseform(const BF & exprForm)
{
	AVM_OS_ASSERT_FATAL_NULL_SMART_POINTER_EXIT( exprForm ) << "expression !!!"
			<< SEND_EXIT;

	switch( exprForm.classKind() )
	{
		case FORM_AVMCODE_KIND:
		{
			const AvmCode & aCode = exprForm.to< AvmCode >();

			switch( aCode.getAvmOpCode() )
			{
				// COMPARISON OPERATION
				case AVM_OPCODE_EQ:
				{
					return( mExprManager.mkExpr(CVC4::kind::EQUAL,
							from_baseform(aCode.first()),
							from_baseform(aCode.second())) );
				}

				case AVM_OPCODE_NEQ:
				{
					return( mExprManager.mkExpr(CVC4::kind::DISTINCT,
							from_baseform(aCode.first()),
							from_baseform(aCode.second())) );
				}

				case AVM_OPCODE_LT:
				{
					if( SolverDef::DEFAULT_SOLVER_KIND ==
							SolverDef::SOLVER_CVC4_BV32_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_SLT,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
					else
//						if( SolverDef::DEFAULT_SOLVER_KIND ==
//								SolverDef::SOLVER_CVC4_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::LT,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
				}

				case AVM_OPCODE_LTE:
				{
					if( SolverDef::DEFAULT_SOLVER_KIND ==
							SolverDef::SOLVER_CVC4_BV32_KIND )
					{
						CVC4::Expr leftMember;
						CVC4::Expr rightMember;
						BFVector rightList;
						BFVector leftList;

						if( aCode.first().is< AvmCode >() &&
							( aCode.first().to< AvmCode >().
									isOpCode( AVM_OPCODE_PLUS ) ) )
						{
							for( const auto & itOperand :
								aCode.first().to< AvmCode >().getOperands() )
							{
								if( itOperand.is< AvmCode >() &&
									( itOperand.to< AvmCode >().
											isOpCode( AVM_OPCODE_UMINUS ) ) )
								{
									rightList.append(
											itOperand.to< AvmCode >().first() );
								}
								else if( itOperand.isInteger() &&
										( itOperand.toInteger() < 0 ) )
								{
									rightList.append( ExpressionConstructor::
											newInteger(- itOperand.toInteger()) );
								}
								else
								{
									leftList.append( itOperand );
								}
							}
						}
						else if( not ( aCode.first().isInteger() &&
									 ( aCode.first().toInteger() == 0 ) ) )
						{
							leftList.append( aCode.first() );
						}

						if( aCode.second().is< AvmCode >() &&
							( aCode.second().to< AvmCode >().
									isOpCode( AVM_OPCODE_PLUS ) ) )
						{
							for( const auto & itOperand :
								aCode.second().to< AvmCode >().getOperands() )
							{
								if( itOperand.is< AvmCode >() &&
									( itOperand.to< AvmCode >().
											isOpCode( AVM_OPCODE_UMINUS ) ) )
								{
									leftList.append(
											itOperand.to< AvmCode >().first() );
								}
								else if( itOperand.isInteger() &&
										( itOperand.toInteger() < 0 ) )
								{
									leftList.append( ExpressionConstructor::
										newInteger(- itOperand.toInteger()) );
								}
								else
								{
									rightList.append( itOperand );
								}
							}
						}
						else if( not ( aCode.second().isInteger() &&
									 ( aCode.second().toInteger() == 0 ) ) )
						{
							rightList.append( aCode.second() );
						}

						if( leftList.empty() )
						{
							leftMember = from_baseform(
									ExpressionConstant::INTEGER_ZERO);
						}
						else if( leftList.singleton() )
						{
							leftMember = from_baseform(leftList[0]);
						}
						else
						{
							// leftMember = BVPLUS des elements de leftList
							leftMember = from_baseform( leftList[0] );
							for (std::size_t i = 1 ; i < leftList.size() ; i ++)
							{
								leftMember =  mExprManager.mkExpr(
										CVC4::kind::BITVECTOR_PLUS, leftMember,
										from_baseform(leftList[i]));
							}
						}

						if( rightList.empty() )
						{
							rightMember = from_baseform(
									ExpressionConstant::INTEGER_ZERO);
						}
						else if( rightList.singleton() )
						{
							rightMember = from_baseform(rightList[0]);
						}
						else
						{
							// rightMember = BVPLUS des elements de rightList
							rightMember = from_baseform( rightList[0] );
							for (std::size_t i = 1 ; i < rightList.size() ; i ++)
							{
								rightMember =  mExprManager.mkExpr(
										CVC4::kind::BITVECTOR_PLUS, rightMember,
										from_baseform(rightList[i]));
							}
						}

						return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_SLE,
								leftMember, rightMember) );
					}
					else
//						if( SolverDef::DEFAULT_SOLVER_KIND ==
//								SolverDef::SOLVER_CVC4_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::LEQ,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
				}

				case AVM_OPCODE_GT:
				{
					if( SolverDef::DEFAULT_SOLVER_KIND ==
							SolverDef::SOLVER_CVC4_BV32_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_SGT,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
					else
//						if( SolverDef::DEFAULT_SOLVER_KIND ==
//								SolverDef::SOLVER_CVC4_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::GT,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
				}

				case AVM_OPCODE_GTE:
				{
					if( SolverDef::DEFAULT_SOLVER_KIND ==
							SolverDef::SOLVER_CVC4_BV32_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_SGE,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
					else
//						if( SolverDef::DEFAULT_SOLVER_KIND ==
//								SolverDef::SOLVER_CVC4_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::GEQ,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
				}


				case AVM_OPCODE_CONTAINS:
				{
					const BuiltinCollection & aCollection =
							aCode.first().to< BuiltinCollection >();

					if( aCollection.singleton() )
					{
						return( mExprManager.mkExpr(CVC4::kind::EQUAL,
								from_baseform(aCollection.at(0)),
								from_baseform(aCode.second())) );
					}
					else if( aCollection.populated() )
					{
						ARGS arg( aCollection.size() );
						const BF & elt = aCode.second();

						for( std::size_t offset = 0 ; arg.hasNext() ; ++offset )
						{

							arg.next( mExprManager.mkExpr(CVC4::kind::EQUAL,
									from_baseform(aCollection.at(offset)),
									from_baseform(elt) ));
						}

						return( mExprManager.mkExpr(CVC4::kind::OR, arg->table) );
					}
					else
					{
						return( SMT_CST_BOOL_FALSE );
					}
				}


				// LOGICAL OPERATION
				case AVM_OPCODE_NOT:
				{
					return( mExprManager.mkExpr(CVC4::kind::NOT,
							from_baseform(aCode.first())) );
				}

				case AVM_OPCODE_AND:
				{
					ARGS arg( aCode.size() );

					for( const auto & itOperand : aCode.getOperands() )
					{
						arg.next( from_baseform(itOperand) );
					}

					return( mExprManager.mkExpr(CVC4::kind::AND, arg->table) );
				}

				case AVM_OPCODE_NAND:
				{
					return( mExprManager.mkExpr(CVC4::kind::NOT,
							mExprManager.mkExpr(CVC4::kind::AND,
									from_baseform(aCode.first()),
									from_baseform(aCode.second()))) );
				}

				case AVM_OPCODE_XAND:
				{
					return( mExprManager.mkExpr(CVC4::kind::OR,
							mExprManager.mkExpr(CVC4::kind::AND,
									from_baseform(aCode.first()),
									from_baseform(aCode.second())),
							mExprManager.mkExpr(CVC4::kind::AND,
									mExprManager.mkExpr(CVC4::kind::NOT,
											from_baseform(aCode.first())),
									mExprManager.mkExpr(CVC4::kind::NOT,
											from_baseform(aCode.second()))) ) );
				}


				case AVM_OPCODE_OR:
				{
					ARGS arg( aCode.size() );

					for( const auto & itOperand : aCode.getOperands() )
					{
						arg.next( from_baseform(itOperand) );
					}

					return( mExprManager.mkExpr(CVC4::kind::OR, arg->table) );
				}

				case AVM_OPCODE_NOR:
				{
					return( mExprManager.mkExpr(CVC4::kind::NOT,
							mExprManager.mkExpr(CVC4::kind::OR,
									from_baseform(aCode.first()),
									from_baseform(aCode.second()))) );
				}

				case AVM_OPCODE_IMPLIES:
				{
					return( mExprManager.mkExpr(CVC4::kind::IMPLIES,
							from_baseform(aCode.first()),
							from_baseform(aCode.second())) );
				}


				// QUANTIFED LOGICAL OPERATION
				case AVM_OPCODE_EXISTS:
				{
					std::size_t boundVarCount = aCode.size()  - 1;

					ARGS boundVars( boundVarCount );

					ARGS boundVarConstraints( aCode.size() );

					for (std::size_t offset = 0; offset < boundVarCount; ++offset)
					{
						boundVars.next(
								getBoundParameterExpr(aCode[ offset ],
										boundVarConstraints) );
					}

					CVC4::Expr boundVarList = mExprManager.mkExpr(
							CVC4::kind::BOUND_VAR_LIST, boundVars->table);

					boundVarConstraints.next(
							from_baseform(aCode[ boundVarCount ]) );

					return( mExprManager.mkExpr(
							CVC4::kind::EXISTS, boundVarList,
							mExprManager.mkExpr(CVC4::kind::AND,
									boundVarConstraints->table)) );

				}

				case AVM_OPCODE_FORALL:
				{
					std::size_t boundVarCount = aCode.size()  - 1;

					ARGS boundVars( boundVarCount );

					ARGS boundVarConstraints( boundVarCount );

					for (std::size_t offset = 0; offset < boundVarCount; ++offset)
					{
						boundVars.next(
								getBoundParameterExpr(aCode[ offset ],
										boundVarConstraints) );
					}

					CVC4::Expr boundVarList = mExprManager.mkExpr(
							CVC4::kind::BOUND_VAR_LIST, boundVars->table);

					CVC4::Expr forallCondition =
							(boundVarCount == 1) ? boundVarConstraints[0]
							: mExprManager.mkExpr(CVC4::kind::AND,
									boundVarConstraints->table);

					forallCondition = mExprManager.mkExpr(
							CVC4::kind::IMPLIES, forallCondition,
							from_baseform(aCode[ boundVarCount ]) );

					return( mExprManager.mkExpr(CVC4::kind::FORALL,
							boundVarList, forallCondition) );
				}


				// BITWISE OPERATION
				case AVM_OPCODE_BAND:
				{
					return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_AND,
							from_baseform(aCode.first()),
							from_baseform(aCode.second())) );
				}

				case AVM_OPCODE_BOR:
				{
					return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_OR,
							from_baseform(aCode.first()),
							from_baseform(aCode.second())) );
				}

				case AVM_OPCODE_LSHIFT:
				{
					if( aCode.second().isInteger() )
					{
						return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_SHL,
								from_baseform(aCode.first()),
								mExprManager.mkConst( CVC4::Rational(
										static_cast<unsigned long int>(
											aCode.second().toInteger())) ) ) );
					}
					else
					{
						AVM_OS_FATAL_ERROR_EXIT
								<< "Unexpected second argument for "
									"newFixedLeftShiftExpr !!!\n"
								<< aCode.toString( AVM_TAB1_INDENT )
								<< SEND_EXIT;

						break;
					}
				}

				case AVM_OPCODE_RSHIFT:
				{
					if( aCode.second().isInteger() )
					{
						return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_ASHR,
								from_baseform(aCode.first()),
								mExprManager.mkConst( CVC4::Rational(
										static_cast<unsigned long int>(
											aCode.second().toInteger())) ) ) );
					}
					else
					{
						AVM_OS_FATAL_ERROR_EXIT
								<< "Unexpected second argument for "
									"newFixedRightShiftExpr !!!\n"
								<< aCode.toString( AVM_TAB1_INDENT )
								<< SEND_EXIT;

						break;
					}
				}

				case AVM_OPCODE_XOR:
				{
					return( mExprManager.mkExpr(CVC4::kind::OR,
							mExprManager.mkExpr(CVC4::kind::AND,
								from_baseform(aCode.first()),
								mExprManager.mkExpr(CVC4::kind::NOT,
									from_baseform(aCode.second()))),
							mExprManager.mkExpr(CVC4::kind::AND,
								mExprManager.mkExpr(CVC4::kind::NOT,
									from_baseform(aCode.first())),
								from_baseform(aCode.second())) ) );
				}

				case AVM_OPCODE_XNOR:
				{
					return( mExprManager.mkExpr(CVC4::kind::OR,
							mExprManager.mkExpr(CVC4::kind::AND,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())),
							mExprManager.mkExpr(CVC4::kind::AND,
								mExprManager.mkExpr(CVC4::kind::NOT,
									from_baseform(aCode.first())),
								mExprManager.mkExpr(CVC4::kind::NOT,
									from_baseform(aCode.second()))) ) );
				}


				// ARITHMETIC OPERATION
				case AVM_OPCODE_PLUS:
				{
					ARGS arg( aCode.size() );

					for( const auto & itOperand : aCode.getOperands() )
					{
						arg.next( from_baseform(itOperand) );
					}

					if( SolverDef::DEFAULT_SOLVER_KIND ==
							SolverDef::SOLVER_CVC4_BV32_KIND )
					{
						return( mExprManager.mkExpr(
								CVC4::kind::BITVECTOR_PLUS, arg->table) );
					}
					else
//						if( SolverDef::DEFAULT_SOLVER_KIND ==
//								SolverDef::SOLVER_CVC4_KIND )
					{
						return( mExprManager.mkExpr(
								CVC4::kind::PLUS, arg->table) );
					}
				}

				case AVM_OPCODE_UMINUS:
				{
					return( mExprManager.mkExpr(CVC4::kind::UMINUS,
							from_baseform(aCode.first())) );
				}

				case AVM_OPCODE_MINUS:
				{
					if( SolverDef::DEFAULT_SOLVER_KIND ==
							SolverDef::SOLVER_CVC4_BV32_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::BITVECTOR_SUB,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
					else
//						if( SolverDef::DEFAULT_SOLVER_KIND ==
//								SolverDef::SOLVER_CVC4_KIND )
					{
						return( mExprManager.mkExpr(CVC4::kind::MINUS,
								from_baseform(aCode.first()),
								from_baseform(aCode.second())) );
					}
				}

				case AVM_OPCODE_MULT:
				{
					ARGS arg( aCode.size() );

					for( const auto & itOperand : aCode.getOperands() )
					{
						arg.next( from_baseform(itOperand) );
					}

					if( SolverDef::DEFAULT_SOLVER_KIND ==
							SolverDef::SOLVER_CVC4_BV32_KIND )
					{
						return( mExprManager.mkExpr(
									CVC4::kind::BITVECTOR_MULT, arg->table) );
					}
					else
//						if( SolverDef::DEFAULT_SOLVER_KIND ==
//								SolverDef::SOLVER_CVC4_KIND )
					{
						return( mExprManager.mkExpr(
								CVC4::kind::MULT, arg->table) );
					}
				}

				case AVM_OPCODE_DIV:
				{
					return( mExprManager.mkExpr(CVC4::kind::DIVISION,
							from_baseform(aCode.first()),
							from_baseform(aCode.second())) );
				}

				case AVM_OPCODE_POW:
				{
					return( mExprManager.mkExpr(CVC4::kind::POW,
							from_baseform(aCode.first()),
							from_baseform(aCode.second())) );
				}

//				case AVM_OPCODE_MOD:

				default:
				{
					AVM_OS_FATAL_ERROR_EXIT
							<< "CVC4Solver::from_baseform:> "
								"Unsupported expression !!!\n"
							<< aCode.toString( AVM_TAB1_INDENT )
							<< SEND_EXIT;

					break;
				}
			}

			break;
		}

		case FORM_INSTANCE_DATA_KIND:
		{
			return( getParameterExpr( exprForm ) );
		}

		case FORM_BUILTIN_BOOLEAN_KIND:
		{
			return( exprForm.to< Boolean >().getValue() ?
					SMT_CST_BOOL_TRUE : SMT_CST_BOOL_FALSE );
		}


		case FORM_BUILTIN_INTEGER_KIND:
		{
			if( SolverDef::DEFAULT_SOLVER_KIND ==
					SolverDef::SOLVER_CVC4_BV32_KIND )
			{
				return( mExprManager.mkConst( CVC4::BitVector( 32,
						static_cast<unsigned long int>(
								exprForm.to< Integer >().toInteger())) ) );
			}
			else
//				if( SolverDef::DEFAULT_SOLVER_KIND == SolverDef::SOLVER_CVC4_KIND )
			{
#if defined( _AVM_BUILTIN_NUMERIC_GMP_ )

				return( mExprManager.mkConst( CVC4::Rational( CVC4::Integer(
						exprForm.to< Integer >().getValue() ) ) ) );

#else

				return( mExprManager.mkConst( CVC4::Rational(
						exprForm.to< Integer >().str()) ) );

#endif /* _AVM_BUILTIN_NUMERIC_GMP_ */
			}
//			else
//			{
//			AVM_OS_FATAL_ERROR_EXIT
//					<< "SolverDef::DEFAULT_SOLVER_KIND <> "
//						"CVC4_INT and <> CVC4_BV32 !!!\n"
//					<< SEND_EXIT;
//
//				return( mExprManager.mkConst( CVC4::Rational(
//						exprForm.to< Integer >().getValue() ) ) );
//			}
		}

		case FORM_BUILTIN_RATIONAL_KIND:
		{
#if defined( _AVM_BUILTIN_NUMERIC_GMP_ )

			return( mExprManager.mkConst( CVC4::Rational(
					exprForm.to< Rational >().getValue() ) ) );

#else

			return( mExprManager.mkConst( CVC4::Rational(
					exprForm.to< Rational >().str() ) ) );

//			return( mExprManager.mkConst( CVC4::Rational(
//					exprForm.to< Rational >().getNumerator(),
//					exprForm.to< Rational >().getDenominator()) ) );
//
//			return( mExprManager.mkConst( CVC4::Rational(
//					exprForm.to< Rational >().strNumerator(),
//					exprForm.to< Rational >().strDenominator()) ) );

#endif /* _AVM_BUILTIN_NUMERIC_GMP_  */
		}

		case FORM_BUILTIN_FLOAT_KIND:
		{
#if defined( _AVM_BUILTIN_NUMERIC_GMP_ )

			return( mExprManager.mkConst( CVC4::Rational( mpq_class(
					exprForm.to< Float >().getValue() ) ) ) );

#else

			return( mExprManager.mkConst(
					CVC4::Rational(exprForm.str()) ) );

#endif /* _AVM_BUILTIN_NUMERIC_GMP_ */
		}


		case FORM_RUNTIME_ID_KIND:
		{
			return( mExprManager.mkConst(
					CVC4::Rational( exprForm.bfRID().getRid() ) ) );
		}


		case FORM_BUILTIN_CHARACTER_KIND:
		{
			AVM_OS_ERROR_ALERT << "Unexpected a CHAR as expression << "
					<< exprForm.to< Character >().getValue()
					<< " >> !!!"
					<< SEND_ALERT;

			return( mExprManager.mkConst( CVC4::Rational(
					exprForm.to< Character >().getValue() ) ) );
		}

		case FORM_BUILTIN_STRING_KIND:
		{
			return( mExprManager.mkConst( CVC4::String(
					exprForm.to< String >().getValue() ) ) );
		}

		case FORM_BUILTIN_QUALIFIED_IDENTIFIER_KIND:
		{
			AVM_OS_FATAL_ERROR_EXIT
					<< "Unexpected a STRING | UFI as expression << "
					<< exprForm.to< String >().getValue() << " >> !!!"
					<< SEND_EXIT;

			break;
//			return( mExprManager.mkConst(
//					exprForm.to< String >().getValue() ) );
		}

		case FORM_BUILTIN_IDENTIFIER_KIND:
		{
			AVM_OS_FATAL_ERROR_EXIT
					<< "Unexpected a IDENTIFIER as expression << "
					<< exprForm.to< Identifier >().getValue()
					<< " >> !!!"
					<< SEND_EXIT;

			break;
//			return( mExprManager.mkConst(
//					exprForm.to< Identifier >().getValue() ) );
		}

		case FORM_OPERATOR_KIND:
		{
			AVM_OS_FATAL_ERROR_EXIT
					<< "Unexpected an OPERATOR as expression << "
					<< exprForm.to< Operator >().standardSymbol()
					<< " >> !!!"
					<< SEND_EXIT;

			break;
//			return( mExprManager.mkConst(
//					exprForm.to< Operator >().standardSymbol() ) );
		}


		case FORM_INSTANCE_BUFFER_KIND:
		case FORM_INSTANCE_CONNECTOR_KIND:
		case FORM_INSTANCE_MACHINE_KIND:
		case FORM_INSTANCE_PORT_KIND:

		default:
		{
			AVM_OS_FATAL_ERROR_EXIT
					<< "CVC4Solver::from_baseform:> Unexpected OBJECT KIND << "
					<< exprForm.classKindName() << " >> as expression << "
					<< exprForm.str() << " >> !!!"
					<< SEND_EXIT;

			break;
//			return( mExprManager.mkConst(
//					exprForm.to< BaseInstanceForm >().getFullyQualifiedNameID() ) );
		}
	}

	AVM_OS_FATAL_ERROR_EXIT
			<< "CVC4Solver::from_baseform ERROR !!!"
			<< SEND_EXIT;

	return( SMT_CST_INT_ZERO );
}


void CVC4Solver::dbg_smt(const CVC4::Expr & aFormula, bool produceModelOption) const
{
	aFormula.printAst(AVM_OS_TRACE << "\t");
	AVM_OS_TRACE << std::endl;

	std::string fileLocation = ( OSS() << mLogFolderLocation
			<< ( produceModelOption ? "cvc4_get_model_" : "cvc4_check_sat_" )
			<< SOLVER_SESSION_ID << ".smt2" );

	std::ofstream osFile;
	osFile.open(fileLocation, std::ios_base::out);
	if ( osFile.good() )
	{
		osFile << "; Getting info" << std::endl
				<< "(get-info :name)"    << std::endl
				<< "(get-info :version)" << std::endl;

		if( produceModelOption )
		{
			osFile << "; Getting values or models" << std::endl
					<< "(set-option :produce-models true)" << std::endl
//					<< "; Getting assignments" << std::endl
//					<< "(set-option :produce-assignments true)" << std::endl
					<< "; Logic" << std::endl
					<< "(set-logic ALL)" << std::endl
					<< std::endl;

			aFormula.printAst(osFile << "\t");

			osFile << std::endl << std::endl
					<< "(get-model)" << std::endl
//					<< "(get-assignment)"
					<< std::endl;
		}
		else
		{
			osFile << std::endl;

			aFormula.printAst(osFile << "\t");

			osFile << std::endl << std::endl
					<< "CHECKSAT;" << std::endl;
		}

		osFile.close();
	}
}




} /* namespace sep */


#endif /* _AVM_SOLVER_CVC4_ */
