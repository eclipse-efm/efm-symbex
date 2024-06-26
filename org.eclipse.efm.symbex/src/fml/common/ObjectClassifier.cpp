/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 21 févr. 2017
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and Implementation
 ******************************************************************************/

#include "ObjectClassifier.h"

#include <fml/infrastructure/Machine.h>


namespace sep
{

bool ObjectClassifier::hasContainerMachine() const
{
	return( (mContainer != nullptr) && mContainer->is< Machine >() );
}


const Machine * ObjectClassifier::getContainerMachine() const
{
	return( mContainer->as_ptr< Machine >() );
}


} /* namespace sep */
