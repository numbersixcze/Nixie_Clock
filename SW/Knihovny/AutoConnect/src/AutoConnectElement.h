/**
 * Alias declarations for an accessible the AutoConnectElement class.
 * @file AutoConnectAux.h
 * @author hieromon@gmail.com
 * @version  0.9.7
 * @date 2018-11-17
 * @copyright  MIT license.
 */

#ifndef _AUTOCONNECTELEMENT_H_
#define _AUTOCONENCTELEMENT_H_

#include "AutoConnectElementBasis.h"
#ifdef AUTOCONNECT_USE_JSON
#include <ArduinoJson.h>
#include "AutoConnectElementJson.h"
using AutoConnectElement  = AutoConnectElementJson;
using AutoConnectButton   = AutoConnectButtonJson;
using AutoConnectCheckbox = AutoConnectCheckboxJson;
using AutoConnectInput    = AutoConnectInputJson;
using AutoConnectRadio    = AutoConnectRadioJson;
using AutoConnectSelect   = AutoConnectSelectJson;
using AutoConnectSubmit   = AutoConnectSubmitJson;
using AutoConnectText     = AutoConnectTextJson;
#define AUTOCONNECT_JSON_BUFFER_SIZE  256
#else
using AutoConnectElement  = AutoConnectElementBasis;
using AutoConnectButton   = AutoConnectButtonBasis;
using AutoConnectCheckbox = AutoConnectCheckboxBasis;
using AutoConnectInput    = AutoConnectInputBasis;
using AutoConnectRadio    = AutoConnectRadioBasis;
using AutoConnectSelect   = AutoConnectSelectBasis;
using AutoConnectSubmit   = AutoConnectSubmitBasis;
using AutoConnectText     = AutoConnectTextBasis;
#endif // !AUTOCONNECT_USE_JSON

/**
 * Support declare the AutoConnectElement variable with reducing the
 * arguments. These macros declare the AutoConnectElement variable
 * with the same name as a "name" argument.
 */
#define ACElement(n, v)    AutoConnectElement n(#n, v)
#define ACButton(n, ...)   AutoConnectButton n(#n, ##__VA_ARGS__)
#define ACCheckbox(n, ...) AutoConnectCheckbox n(#n, ##__VA_ARGS__)
#define ACInput(n, ...)    AutoConnectInput n(#n, ##__VA_ARGS__)
#define ACRadio(n, ...)    AutoConnectRadio n(#n, ##__VA_ARGS__)
#define ACSelect(n, ...)   AutoConnectSelect n(#n, ##__VA_ARGS__)
#define ACSubmit(n, ...)   AutoConnectSubmit n(#n, ##__VA_ARGS__)
#define ACText(n, ...)     AutoConnectText n(#n, ##__VA_ARGS__)

#endif // _AUTOCONNECTELEMENT_H_
