/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 16 mai 2016
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and Implementation
 ******************************************************************************/

#ifndef FML_NUMERIC_BASIC_NUMERICIMPL_H_
#define FML_NUMERIC_BASIC_NUMERICIMPL_H_

#include <fml/numeric/basic/IntegerImpl.h>
#include <fml/numeric/basic/RationalImpl.h>
#include <fml/numeric/basic/FloatImpl.h>


namespace sep
{
namespace numeric
{


/**
 * compare Float
 * with ...
 */
inline int compare(const Float & num1, const Float & num2)
{
	return( num1.compare( num2 ) );
}

inline int compare(const Float & num1, const Integer & num2)
{
	return( num1.compare( num2 ) );
}

inline int compare(const Float & num1, const Rational & num2)
{
	return( num1.compare( Float( num2.toFloat() ) ) );
}


/**
 * compare Rational
 * with ...
 */
inline int compare(const Rational & num1, const Rational & num2)
{
	return( num1.compare( num2 ) );
}

inline int compare(const Rational & num1, const Integer & num2)
{
	return( num1.compare( num2 ) );
}

inline int compare(const Rational & num1, const Float & num2)
{
	return( - compare( num2, num1 ) );
}


/**
 * compare Integer
 * with ...
 */
inline int compare(const Integer & num1, const Integer & num2)
{
	return( num1.compare( num2 ) );
}

inline int compare(const Integer & num1, const Rational & num2)
{
	return( - compare( num2, num1 ) );
}

inline int compare(const Integer & num1, const Float & num2)
{
	return( - compare( num2, num1 ) );
}


/**
 * operator==
 */
// Integer == Number
inline bool operator==(const Integer & num1, const Integer & num2)
{
	return( num1.getValue() == num2.getValue() );
}

inline bool operator==(const Integer & num1, const Rational & num2)
{
	return( num1.getValue() == num2.getValue() );
}

inline bool operator==(const Integer & num1, const Float & num2)
{
	return( num1.getValue() == num2.getValue() );
}


// Rational == Number
inline bool operator==(const Rational & num1, const Rational & num2)
{
	return( num1.getValue() == num2.getValue() );
}

inline bool operator==(const Rational & num1, const Integer & num2)
{
	return( num1.getValue() == num2.getValue() );
}

inline bool operator==(const Rational & num1, const Float & num2)
{
	return( num1.toFloat() == num2.getValue() );
}


// Float == Number
inline bool operator==(const Float & num1, const Float & num2)
{
	return( num1.getValue() == num2.getValue() );
}

inline bool operator==(const Float & num1, const Rational & num2)
{
	return( num1.getValue() == num2.toFloat() );
}

inline bool operator==(const Float & num1, const Integer & num2)
{
	return( num1.getValue() == num2.getValue() );
}


/**
 * operator!=
 */
// Integer != Number
inline bool operator!=(const Integer & num1, const Integer & num2)
{
	return( num1.getValue() != num2.getValue() );
}

inline bool operator!=(const Integer & num1, const Rational & num2)
{
	return( num1.getValue() != num2.getValue() );
}

inline bool operator!=(const Integer & num1, const Float & num2)
{
	return( num1.getValue() != num2.getValue() );
}


// Rational != Number
inline bool operator!=(const Rational & num1, const Rational & num2)
{
	return( num1.getValue() != num2.getValue() );
}

inline bool operator!=(const Rational & num1, const Integer & num2)
{
	return( num1.getValue() != num2.getValue() );
}

inline bool operator!=(const Rational & num1, const Float & num2)
{
	return( num1.toFloat() != num2.getValue() );
}


// Float != Number
inline bool operator!=(const Float & num1, const Float & num2)
{
	return( num1.getValue() != num2.getValue() );
}

inline bool operator!=(const Float & num1, const Rational & num2)
{
	return( num1.getValue() != num2.toFloat() );
}

inline bool operator!=(const Float & num1, const Integer & num2)
{
	return( num1.getValue() != num2.getValue() );
}


/**
 * operator<
 */
// Integer < Number
inline bool operator<(const Integer & num1, const Integer & num2)
{
	return( num1.getValue() < num2.getValue() );
}

inline bool operator<(const Integer & num1, const Rational & num2)
{
	return( num1.getValue() < num2.getValue() );
}

inline bool operator<(const Integer & num1, const Float & num2)
{
	return( num1.getValue() < num2.getValue() );
}


// Rational < Number
inline bool operator<(const Rational & num1, const Rational & num2)
{
	return( num1.getValue() < num2.getValue() );
}

inline bool operator<(const Rational & num1, const Integer & num2)
{
	return( num1.getValue() < num2.getValue() );
}

inline bool operator<(const Rational & num1, const Float & num2)
{
	return( num1.toFloat() < num2.getValue() );
}


// Float < Number
inline bool operator<(const Float & num1, const Float & num2)
{
	return( num1.getValue() < num2.getValue() );
}

inline bool operator<(const Float & num1, const Rational & num2)
{
	return( num1.getValue() < num2.toFloat() );
}

inline bool operator<(const Float & num1, const Integer & num2)
{
	return( num1.getValue() < num2.getValue() );
}

/**
 * operator<=
 */
// Integer <= Number
inline bool operator<=(const Integer & num1, const Integer & num2)
{
	return( num1.getValue() <= num2.getValue() );
}

inline bool operator<=(const Integer & num1, const Rational & num2)
{
	return( num1.getValue() <= num2.getValue() );
}

inline bool operator<=(const Integer & num1, const Float & num2)
{
	return( num1.getValue() <= num2.getValue() );
}


// Rational <= Number
inline bool operator<=(const Rational & num1, const Rational & num2)
{
	return( num1.getValue() <= num2.getValue() );
}

inline bool operator<=(const Rational & num1, const Integer & num2)
{
	return( num1.getValue() <= num2.getValue() );
}

inline bool operator<=(const Rational & num1, const Float & num2)
{
	return( num1.toFloat() <= num2.getValue() );
}


// Float <= Number
inline bool operator<=(const Float & num1, const Float & num2)
{
	return( num1.getValue() <= num2.getValue() );
}

inline bool operator<=(const Float & num1, const Rational & num2)
{
	return( num1.getValue() <= num2.toFloat() );
}

inline bool operator<=(const Float & num1, const Integer & num2)
{
	return( num1.getValue() <= num2.getValue() );
}


/**
 * operator>
 */
// Integer > Number
inline bool operator>(const Integer & num1, const Integer & num2)
{
	return( num1.getValue() > num2.getValue() );
}

inline bool operator>(const Integer & num1, const Rational & num2)
{
	return( num1.getValue() > num2.getValue() );
}

inline bool operator>(const Integer & num1, const Float & num2)
{
	return( num1.getValue() > num2.getValue() );
}


// Rational > Number
inline bool operator>(const Rational & num1, const Rational & num2)
{
	return( num1.getValue() > num2.getValue() );
}

inline bool operator>(const Rational & num1, const Integer & num2)
{
	return( num1.getValue() > num2.getValue() );
}

inline bool operator>(const Rational & num1, const Float & num2)
{
	return( num1.toFloat() > num2.getValue() );
}


// Float > Number
inline bool operator>(const Float & num1, const Float & num2)
{
	return( num1.getValue() > num2.getValue() );
}

inline bool operator>(const Float & num1, const Rational & num2)
{
	return( num1.getValue() > num2.toFloat() );
}

inline bool operator>(const Float & num1, const Integer & num2)
{
	return( num1.getValue() > num2.getValue() );
}

/**
 * operator>=
 */
// Integer >= Number
inline bool operator>=(const Integer & num1, const Integer & num2)
{
	return( num1.getValue() >= num2.getValue() );
}

inline bool operator>=(const Integer & num1, const Rational & num2)
{
	return( num1.getValue() >= num2.getValue() );
}

inline bool operator>=(const Integer & num1, const Float & num2)
{
	return( num1.getValue() >= num2.getValue() );
}


// Rational >= Number
inline bool operator>=(const Rational & num1, const Rational & num2)
{
	return( num1.getValue() >= num2.getValue() );
}

inline bool operator>=(const Rational & num1, const Integer & num2)
{
	return( num1.getValue() >= num2.getValue() );
}

inline bool operator>=(const Rational & num1, const Float & num2)
{
	return( num1.toFloat() >= num2.getValue() );
}


// Float >= Number
inline bool operator>=(const Float & num1, const Float & num2)
{
	return( num1.getValue() >= num2.getValue() );
}

inline bool operator>=(const Float & num1, const Rational & num2)
{
	return( num1.getValue() >= num2.toFloat() );
}

inline bool operator>=(const Float & num1, const Integer & num2)
{
	return( num1.getValue() >= num2.getValue() );
}


/**
 * operator+
 */
// Integer + Number
inline Integer operator+(const Integer & num1, const Integer & num2)
{
	return( Integer(num1.getValue() + num2.getValue()) );
}

inline Rational operator+(const Integer & num1, const Rational & num2)
{
	return( Rational(Rational(num1).getValue() + num2.getValue()) );
}

inline Float operator+(const Integer & num1, const Float & num2)
{
	return( Float(num1.getValue() + num2.getValue()) );
}


// Rational + Number
inline Rational operator+(const Rational & num1, const Rational & num2)
{
	return( Rational(num1.getValue() + num2.getValue()) );
}

inline Rational operator+(const Rational & num1, const Integer & num2)
{
	return( Rational(num1.getValue() + Rational(num2).getValue()) );
}

inline Float operator+(const Rational & num1, const Float & num2)
{
	return( Float(num1.toFloat() + num2.getValue()) );
}


// Float + Number
inline Float operator+(const Float & num1, const Float & num2)
{
	return( Float(num1.getValue() + num2.getValue()) );
}

inline Float operator+(const Float & num1, const Rational & num2)
{
	return( Float(num1.getValue() + num2.toFloat()) );
}

inline Float operator+(const Float & num1, const Integer & num2)
{
	return( Float(num1.getValue() + num2.getValue() ) );
}


/**
 * operator-
 */
inline Integer operator-(const Integer & num)
{
	return( Integer(- num.getValue()) );
}

inline Rational operator-(const Rational & num)
{
	return( Rational(- num.getValue()) );
}

inline Float operator-(const Float & num)
{
	return( Float(- num.getValue()) );
}


/**
 * operator-
 */
// Integer - Number
inline Integer operator-(const Integer & num1, const Integer & num2)
{
	return( Integer(num1.getValue() - num2.getValue()) );
}

inline Rational operator-(const Integer & num1, const Rational & num2)
{
	return( Rational(Rational(num1).getValue() - num2.getValue()) );
}

inline Float operator-(const Integer & num1, const Float & num2)
{
	return( Float(num1.getValue() - num2.getValue()) );
}


// Rational - Number
inline Rational operator-(const Rational & num1, const Rational & num2)
{
	return( Rational(num1.getValue() - num2.getValue()) );
}

inline Rational operator-(const Rational & num1, const Integer & num2)
{
	return( Rational(num1.getValue() - Rational(num2).getValue()) );
}

inline Float operator-(const Rational & num1, const Float & num2)
{
	return( Float(num1.toFloat() - num2.getValue()) );
}


// Float - Number
inline Float operator-(const Float & num1, const Float & num2)
{
	return( Float(num1.getValue() - num2.getValue()) );
}

inline Float operator-(const Float & num1, const Rational & num2)
{
	return( Float(num1.getValue() - num2.getValue()) );
}

inline Float operator-(const Float & num1, const Integer & num2)
{
	return( Float(num1.getValue() - num2.getValue() ) );
}


/**
 * operator*
 */
// Integer * Number
inline Integer operator*(const Integer & num1, const Integer & num2)
{
	return( Integer(num1.getValue() * num2.getValue()) );
}

inline Rational operator*(const Integer & num1, const Rational & num2)
{
	return( Rational(num1.getValue() * num2.rawNumerator() ,
			num2.rawDenominator()) );
}

inline Float operator*(const Integer & num1, const Float & num2)
{
	return( Float(num1.getValue() * num2.getValue()) );
}


// Rational * Number
inline Rational operator*(const Rational & num1, const Rational & num2)
{
	return( Rational(num1.getValue() * num2.getValue()) );
}

inline Rational operator*(const Rational & num1, const Integer & num2)
{
	return( Rational(num1.rawNumerator() * num2.getValue(),
			num1.rawDenominator()) );
}

inline Float operator*(const Rational & num1, const Float & num2)
{
	return( Float(num1.toFloat() * num2.getValue()) );
}


// Float * Number
inline Float operator*(const Float & num1, const Float & num2)
{
	return( Float(num1.getValue() * num2.getValue()) );
}

inline Float operator*(const Float & num1, const Rational & num2)
{
	return( Float(num1.getValue() * num2.toFloat()) );
}

inline Float operator*(const Float & num1, const Integer & num2)
{
	return( Float(num1.getValue() * num2.getValue() ) );
}


/**
 * pow
 */
inline Integer pow(const Integer & num, avm_uinteger_t anExponent)
{
	return( Integer( num.pow(anExponent) ) );
}

inline Rational pow(const Rational & num, avm_uinteger_t anExponent)
{
	Rational result( num );
	result.set_pow( anExponent );

	return( Rational( result ) );
}

inline Float pow(const Float & num, avm_uinteger_t anExponent)
{
	Float result( num );
	result.set_pow( anExponent );

	return( Float( result ) );
}


/**
 * operator/
 */
// Integer / Number
inline Rational operator/(const Integer & num1, const Integer & num2)
{
	return( Rational( num1.getValue() , num2.getValue() ) );
}

inline Rational operator/(const Integer & num1, const Rational & num2)
{
	return( Rational(num1.getValue() * num2.rawDenominator() ,
			num2.rawNumerator() ) );
}

inline Float operator/(const Integer & num1, const Float & num2)
{
	return( Float(num1.getValue() / num2.getValue()) );
}


// Rational / Number
inline Rational operator/(const Rational & num1, const Rational & num2)
{
	return( Rational(num1.getValue() / num2.getValue()) );
}

inline Rational operator/(const Rational & num1, const Integer & num2)
{
	return( Rational(num1.rawNumerator() ,
			num1.rawDenominator() * num2.getValue()) );
}

inline Float operator/(const Rational & num1, const Float & num2)
{
	return( Float(num1.toFloat() / num2.getValue()) );
}

// Float / Number
inline Float operator/(const Float & num1, const Float & num2)
{
	return( Float(num1.getValue() / num2.getValue()) );
}

inline Float operator/(const Float & num1, const Rational & num2)
{
	return( Float(num1.getValue() / num2.toFloat()) );
}

inline Float operator/(const Float & num1, const Integer & num2)
{
	return( Float(num1.getValue() / num2.getValue() ) );
}


/**
 * inverse
 */
inline Rational inverse(const Integer & num)
{
	return( Rational( Integer(1) , num ) );
}

inline Rational inverse(const Rational & num)
{
	return( Rational( num.rawDenominator() , num.rawNumerator() ) );
}

inline Float inverse(const Float & num)
{
	return( Float( 1 / num.getValue() ) );
}


/**
 * operator%
 */
// Integer % Integer
inline Integer operator%(const Integer & num1, const Integer & num2)
{
	return( Integer(num1.getValue() % num2.getValue()) );
}



} /* namespace numeric */
} /* namespace sep */

#endif /* FML_NUMERIC_BASIC_NUMERICIMPL_H_ */
