//
// Created by Benoît on 15/02/2023.
//

#ifndef MIXRTS_MSVCMACROS_HPP
#define MIXRTS_MSVCMACROS_HPP

#ifdef _MSC_VER
#ifdef MYDLL_EXPORTS
#define MYDLL_API __declspec(dllexport)
#else
#define MYDLL_API __declspec(dllimport)
#endif
#else
#define MYDLL_API
#endif

#endif //MIXRTS_MSVCMACROS_HPP
