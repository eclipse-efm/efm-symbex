/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 27 nov. 2012
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#ifndef AVMOPERATIONEXPRESSION_H_
#define AVMOPERATIONEXPRESSION_H_

#include <map>

#include <fml/operator/Operator.h>


namespace sep
{


class BF;


class AvmOperationExpression
{

public:
	/**
	 * LOADER - DISPOSER
	 */
	static void load();
	static void dispose();


	/**
	 * GETTER - SETTER
	 * theOtherMap
	 */
	static std::map< std::string , const Operator * > theOtherMap;

	inline static const Operator * getOther(const std::string & method)
	{
		return( theOtherMap[ method ] );
	}

	inline static bool isOther(const std::string & method)
	{
		return( theOtherMap.find( method ) != theOtherMap.end() );
	}

	inline static void putOther(
			const std::string & method, const Operator * anOperator)
	{
		theOtherMap[ method ] = anOperator;
	}



	/**
	 * GETTER - SETTER
	 */
	inline static const Operator * get(const std::string & method)
	{
		return( getOther(method) );
	}

	inline static const Operator * get(
			const BF & aReceiver, const std::string & method)
	{
		return( get(method) );
	}

	inline static bool exists(const std::string & method)
	{
		return( isOther(method) );
	}

	inline static bool exists(
			const std::string & method, const Operator * anOperator)
	{
		return( anOperator == getOther(method) );
	}


	inline static void put(
			const std::string & method, const Operator * anOperator)
	{
		putOther(method, anOperator);
	}


};



} /* namespace sep */
#endif /* AVMOPERATIONEXPRESSION_H_ */
