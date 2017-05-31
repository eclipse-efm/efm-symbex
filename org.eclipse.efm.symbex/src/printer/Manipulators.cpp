/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 2 juin 2016
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and Implementation
 ******************************************************************************/

#include "Manipulators.h"

namespace sep {


/**
 * DEFAULT VALUE CSS
 */
SymbexValueCSS DEFAULT_SYMBEX_VALUE_ARRAY_CSS ( "[ ", " , ", " ]" );

SymbexValueCSS DEFAULT_SYMBEX_VALUE_PARAMS_CSS ( "( ", " , ", " )" );

SymbexValueCSS DEFAULT_SYMBEX_VALUE_STRUCT_CSS( "{ ", " , ", " }" );


/**
 * DEFAULT AVM INDENT
 */
AvmIndent AVM_TAB_INDENT ( ""   , "\t" , "\n" );

AvmIndent AVM_TAB1_INDENT( "\t" , "\t" , "\n" );


AvmIndent AVM_SPC_INDENT ( ""   , " "  , "\n" );

AvmIndent AVM_SPC1_INDENT( " "  , " "  , "\n" );


AvmIndent AVM_STR_INDENT( " "   , ""   , ""   );

AvmIndent AVM_RTS_INDENT( ""    , ""   , " "  );

AvmIndent AVM_NO_INDENT ( ""    , ""   , ""   );

// Default output / fscn file indent
AvmIndent AVM_OUTPUT_INDENT( "" , "\t" , "\n" );

AvmIndent AVM_FSCN_INDENT  ( "" , "\t" , "\n" );


} /* namespace sep */
