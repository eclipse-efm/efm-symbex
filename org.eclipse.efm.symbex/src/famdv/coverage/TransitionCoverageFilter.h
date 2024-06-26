/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 25 juil. 2008
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *  Alain Faivre (CEA LIST) alain.faivre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#ifndef FAM_COVERAGE_TRANSITIONCOVERAGEFILTER_H_
#define FAM_COVERAGE_TRANSITIONCOVERAGEFILTER_H_

#include <collection/Bitset.h>
#include <collection/Typedef.h>
#include <famcore/api/BaseCoverageFilter.h>

#include  <famdv/coverage/AvmCoverageTraceDriver.h>

#include <fml/common/SpecifierElement.h>


namespace sep
{

class WaitingStrategy;
class AvmTransition;
class ExecutionData;
class RuntimeID;
class SymbexControllerUnitManager;


class TransitionCoverageFilter :
		public AutoRegisteredCoverageProcessorUnit< TransitionCoverageFilter >
{

	AVM_DECLARE_CLONABLE_CLASS( TransitionCoverageFilter )


	/**
	 * PROCESSOR FACTORY
	 * for automatic registration in the processor repository
	 * the [ [ FULLY ] QUALIFIED ] NAME ID
	 */
	AVM_INJECT_AUTO_REGISTER_QUALIFIED_ID_KEY_4(
			"coverage#transition",
			"avm::processor.TRANSITION_COVERAGE",
			"avm::core.filter.PROGRAM_COVERAGE",
			"avm::core.filter.TRANSITION_COVERAGE")
	// end registration


protected:
	/**
	 * TYPEDEF
	 */
	typedef Vector< const ExecutableForm * >  VectorOfExecutableForm;

	/**
	 * ATTRIBUTES
	 */
	Specifier::DESIGN_KIND mScope;

	std::size_t mNbCoverage;

	std::size_t mCoverageHeightPeriod;
	std::size_t mCoverageHeight;
	std::size_t mCoverageHeightReachedCount;
	std::size_t mCoverageHeightReachedLimit;
	bool       mCoverageHeightReachedFlag;


	bool mHitStronglyRandomFlag;
	std::size_t mHitStronglyCount;

	bool mHitWeaklyRandomFlag;
	std::size_t mHitWeaklyCount;

	bool mHitOtherRandomFlag;
	std::size_t mHitOtherCount;

	// Table des flags de couverture de transition
	// pour chaque instance de machine executable
	ArrayOfBitset * mExecutableCoverageTable;
	ArrayOfBitset * mTransitionCoverageTable;

	VectorOfExecutableForm mTableofRuntimeExecutable;


	VectorOfAvmTransition mTableofUncoveredTransition;
	std::size_t mLastCollectedCoverCount;


	// Utilisation d'une trace pour orienté/dirigé l'exécution vers une cible
	bool mTraceDirectiveRunningFlag;
	AvmCoverageTraceDriver mTraceDriver;

	std::size_t mLastDirectiveTransitionOffset;

	// Computing Local Variables
	Bitset * tmpRuntimeTransitionBitset;

	avm_offset_t offset;
	avm_offset_t endOffset;

	avm_offset_t maxRandomOffset;
	Bitset randomOffsetBitset;


	ExecutionContext::child_iterator itEC;
	ExecutionContext::child_iterator itEndEC;
	ExecutionContext * tmpEC;
	const ExecutableForm * tmpEF;
	RuntimeID itRID;
	TableOfRuntimeT::const_iterator itRF;
	TableOfRuntimeT::const_iterator endRF;


	VectorOfExecutionContext mStronglyHitEC;
	std::size_t mStronglyFireableTransitionCount;
	std::size_t tmpStronglyFireableTransitionCount;

	VectorOfExecutionContext mWeaklyHitEC;
	std::size_t mWeaklyFireableTransitionCount;
	std::size_t tmpWeaklyFireableTransitionCount;

	ListOfExecutionContext mWaitingQueue;


public:

	/**
	 * CONSTRUCTOR
	 * Default
	 */
	TransitionCoverageFilter(
			SymbexControllerUnitManager & aControllerUnitManager,
			const WObject * wfParameterObject)
	: RegisteredCoverageProcessorUnit(aControllerUnitManager, wfParameterObject,
			AVM_PREPOST_FILTERING_STAGE, PRECEDENCE_OF_TRANSITION_COVERAGE),
	mScope( Specifier::DESIGN_MODEL_KIND ),

	mNbCoverage( AVM_NUMERIC_MAX_SIZE_T ),

	mCoverageHeightPeriod( 0 ),
	mCoverageHeight( 0 ),
	mCoverageHeightReachedCount( 0 ),
	mCoverageHeightReachedLimit( 1 ),
	mCoverageHeightReachedFlag( false ),

	mHitStronglyRandomFlag( false ),
	mHitStronglyCount( 1 ),

	mHitWeaklyRandomFlag( false ),
	mHitWeaklyCount( 1 ),

	mHitOtherRandomFlag( false ),
	mHitOtherCount( 1 ),

	mExecutableCoverageTable( nullptr ),
	mTransitionCoverageTable( nullptr ),
	mTableofRuntimeExecutable( ),

	mTableofUncoveredTransition( ),
	mLastCollectedCoverCount( 0 ),

	mTraceDirectiveRunningFlag( false ),
	mTraceDriver( ENV ),

	mLastDirectiveTransitionOffset( 0 ),

	// Computing Local Variables
	tmpRuntimeTransitionBitset( nullptr ),

	offset( 0 ),
	endOffset( 0 ),

	maxRandomOffset( 0 ),
	randomOffsetBitset( ),

	itEC( ),
	itEndEC( ),
	tmpEC( nullptr ),
	tmpEF( nullptr ),
	itRID( ),
	itRF( ),
	endRF( ),

	mStronglyHitEC( ),
	mStronglyFireableTransitionCount( 0 ),
	tmpStronglyFireableTransitionCount( 0 ),

	mWeaklyHitEC( ),
	mWeaklyFireableTransitionCount( 0 ),
	tmpWeaklyFireableTransitionCount( 0 ),

	mWaitingQueue( )
	{
		//!! NOTHING
	}

	/**
	 * DESTRUCTOR
	 */
	virtual ~TransitionCoverageFilter();


	/**
	 * CONFIGURE
	 */
	virtual bool configureImpl() override;


	void configureExecutableCoverageTableFlag(bool value);
	void configureExecutableCoverageTableFlag(
			const ExecutableForm & anExecutable, bool value);

	void configureInstanceCoverageTableFlag();
	void configureInstanceCoverageTableFlag(bool value);
	void configureInstanceCoverageTableFlag(
			const ExecutionData & anED, const RuntimeID & aRID, bool value);

	void configureTransitionCoverageTableFlag(
			ListOfAvmTransition & listOfTransition, bool value);


	Bitset * getCoverageTableBitset(const RuntimeID & aRID);


	/**
	 * REPORT TRACE
	 */
	virtual void reportMinimum(OutStream & os) const override;

	virtual void reportDefault(OutStream & os) const override;

	////////////////////////////////////////////////////////////////////////////
	// NON-REGRESSION TEST API
	////////////////////////////////////////////////////////////////////////////

	virtual void tddRegressionReportImpl(OutStream & os) const override;


	/**
	 * postEval Filter
	 */
	virtual bool prefilter() override;

	virtual bool postfilter() override;

	// REQUEUE REQUEST for the Waiting Queue Table
	virtual void handleRequestRequeueWaitingTable(
			WaitingStrategy & aWaitingStrategy,
			std::uint8_t aWeightMin, std::uint8_t aWeightMax) override;

	/**
	 * Heuristic Class implementation
	 */
	void heuristicNaiveClassImpl();

	void heuristicSmartClassImpl();

	void heuristicAgressiveClassImpl();

	void heuristicNothingElseClassImpl();


	void setHitWeight(VectorOfExecutionContext & hitEC,
			std::uint8_t hitWeight, bool randomFlag, std::size_t hitCount);

	void computeWeight(ExecutionContext * anEC);
	void computeWeightNaive(ExecutionContext * anEC);
	void computeWeightSmart(ExecutionContext * anEC);
	void computeWeightAgressive(ExecutionContext * anEC);


	bool computeCheckNonPriorityWeight(ExecutionContext * anEC);

	void computePriorityWeight(ExecutionContext * anEC);

	bool checkStronglyPriorityWeight(ExecutionContext * anEC);
	bool checkWeaklyPriorityWeight(ExecutionContext * anEC);


	void computeWeightOfResult();


	bool testFireability(const ExecutionData & anED,
			const RuntimeID & aReceiverRID, AvmTransition * aTransition);

	bool isControlLoop(ExecutionContext * anEC);
	bool isSyntaxicLoop(ExecutionContext * anEC);
	bool isTrivialLoop(ExecutionContext * anEC);

	void fireableTransitionCount(
			const ExecutionData & anED, const RuntimeID & aRID);

	void fireableTransitionCount(const ExecutionData & anED);

	void fireableTransitionCount(
			const ExecutionData & anED, const RuntimeForm & aRF);

	void fireableTransitionTrace(OutStream & os, const ExecutionData & anED);

	void fireableTransitionTrace(OutStream & os,
			const ExecutionData & anED, const RuntimeForm & aRF);

	void updateTransitionCoverageTable(
			ExecutionContext * anEC, const BF & aFiredCode);

	void updateTransitionCoverageTable(ExecutionContext * anEC,
			const RuntimeID & aRID, AvmTransition * firedTransition);

	bool testTransitionCoverage(const AvmTransition * firedTransition);

	/**
	 * mTableofUncoveredTransition
	 */
	void collectUncoveredTransition();

	void collectUncoveredTransition(const ExecutionData & anED,
			ListOfAvmTransition & listofTransition);


	/**
	 * mTraceDriver
	 */
	bool initializeTraceDriver(WaitingStrategy & aWaitingStrategy);

	bool initializeTraceDriver(ExecutionContext * anEC);

	bool runTraceDriver();

};


}

#endif /* FAM_COVERAGE_TRANSITIONCOVERAGEFILTER_H_ */
