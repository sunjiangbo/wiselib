/***************************************************************************
 ** Copyright (c) 2005 Sergey Ryazanov (http://home.onego.ru/~ryazanov)   **
 **                                                                       **
 ** Permission is hereby granted, free of charge, to any person           **
 ** obtaining a copy of this software and associated documentation        **
 ** files (the "Software"), to deal in the Software without               **
 ** restriction, including without limitation the rights to use, copy,    **
 ** modify, merge, publish, distribute, sublicense, and/or sell copies    **
 ** of the Software, and to permit persons to whom the Software is        **
 ** furnished to do so, subject to the following conditions:              **
 **                                                                       **
 ** The above copyright notice and this permission notice shall be        **
 ** included in all copies or substantial portions of the Software.       **
 **                                                                       **
 ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       **
 ** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    **
 ** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                 **
 ** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT           **
 ** HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,          **
 ** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,    **
 ** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         **
 ** DEALINGS IN THE SOFTWARE.                                             **
 ***************************************************************************/

/***************************************************************************
 ** As given in
 **   http://www.codeproject.com/Articles/11015/The-Impossibly-Fast-C-Delegates
 ** this file is under the MIT license, see
 **   http://www.opensource.org/licenses/mit-license.php
 ** for details.
 **
 ** Kind regards,
 ** Tobias Baumgartner
 **
 ***************************************************************************/

// 0 params
#define SRUTIL_DELEGATE_PARAM_COUNT 0
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS 
#define SRUTIL_DELEGATE_TEMPLATE_ARGS 
#define SRUTIL_DELEGATE_PARAMS 
#define SRUTIL_DELEGATE_ARGS 
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST 
#define SRUTIL_DELEGATE_INVOKER_DATA 
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 1 params
#define SRUTIL_DELEGATE_PARAM_COUNT 1
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1
#define SRUTIL_DELEGATE_PARAMS A1 a1
#define SRUTIL_DELEGATE_ARGS a1
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 2 params
#define SRUTIL_DELEGATE_PARAM_COUNT 2
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2
#define SRUTIL_DELEGATE_ARGS a1,a2
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 3 params
#define SRUTIL_DELEGATE_PARAM_COUNT 3
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2, typename A3
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2, A3
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2, A3 a3
#define SRUTIL_DELEGATE_ARGS a1,a2,a3
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2),a3(a3)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;A3 a3;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 4 params
#define SRUTIL_DELEGATE_PARAM_COUNT 4
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2, typename A3, typename A4
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2, A3, A4
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2, A3 a3, A4 a4
#define SRUTIL_DELEGATE_ARGS a1,a2,a3,a4
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2),a3(a3),a4(a4)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;A3 a3;A4 a4;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 5 params
#define SRUTIL_DELEGATE_PARAM_COUNT 5
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2, typename A3, typename A4, typename A5
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2, A3, A4, A5
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2, A3 a3, A4 a4, A5 a5
#define SRUTIL_DELEGATE_ARGS a1,a2,a3,a4,a5
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2),a3(a3),a4(a4),a5(a5)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;A3 a3;A4 a4;A5 a5;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 6 params
#define SRUTIL_DELEGATE_PARAM_COUNT 6
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2, typename A3, typename A4, typename A5, typename A6
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2, A3, A4, A5, A6
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6
#define SRUTIL_DELEGATE_ARGS a1,a2,a3,a4,a5,a6
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2),a3(a3),a4(a4),a5(a5),a6(a6)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;A3 a3;A4 a4;A5 a5;A6 a6;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 7 params
#define SRUTIL_DELEGATE_PARAM_COUNT 7
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2, A3, A4, A5, A6, A7
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7
#define SRUTIL_DELEGATE_ARGS a1,a2,a3,a4,a5,a6,a7
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2),a3(a3),a4(a4),a5(a5),a6(a6),a7(a7)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;A3 a3;A4 a4;A5 a5;A6 a6;A7 a7;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 8 params
#define SRUTIL_DELEGATE_PARAM_COUNT 8
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2, A3, A4, A5, A6, A7, A8
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8
#define SRUTIL_DELEGATE_ARGS a1,a2,a3,a4,a5,a6,a7,a8
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2),a3(a3),a4(a4),a5(a5),a6(a6),a7(a7),a8(a8)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;A3 a3;A4 a4;A5 a5;A6 a6;A7 a7;A8 a8;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 9 params
#define SRUTIL_DELEGATE_PARAM_COUNT 9
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2, A3, A4, A5, A6, A7, A8, A9
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9
#define SRUTIL_DELEGATE_ARGS a1,a2,a3,a4,a5,a6,a7,a8,a9
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2),a3(a3),a4(a4),a5(a5),a6(a6),a7(a7),a8(a8),a9(a9)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;A3 a3;A4 a4;A5 a5;A6 a6;A7 a7;A8 a8;A9 a9;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

// 10 params
#define SRUTIL_DELEGATE_PARAM_COUNT 10
#define SRUTIL_DELEGATE_TEMPLATE_PARAMS typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10
#define SRUTIL_DELEGATE_TEMPLATE_ARGS A1, A2, A3, A4, A5, A6, A7, A8, A9, A10
#define SRUTIL_DELEGATE_PARAMS A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10
#define SRUTIL_DELEGATE_ARGS a1,a2,a3,a4,a5,a6,a7,a8,a9,a10
#define SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST a1(a1),a2(a2),a3(a3),a4(a4),a5(a5),a6(a6),a7(a7),a8(a8),a9(a9),a10(a10)
#define SRUTIL_DELEGATE_INVOKER_DATA A1 a1;A2 a2;A3 a3;A4 a4;A5 a5;A6 a6;A7 a7;A8 a8;A9 a9;A10 a10;
#include "delegate_template.hpp"
#undef SRUTIL_DELEGATE_PARAM_COUNT
#undef SRUTIL_DELEGATE_TEMPLATE_PARAMS
#undef SRUTIL_DELEGATE_TEMPLATE_ARGS
#undef SRUTIL_DELEGATE_PARAMS
#undef SRUTIL_DELEGATE_ARGS
#undef SRUTIL_DELEGATE_INVOKER_INITIALIZATION_LIST
#undef SRUTIL_DELEGATE_INVOKER_DATA

