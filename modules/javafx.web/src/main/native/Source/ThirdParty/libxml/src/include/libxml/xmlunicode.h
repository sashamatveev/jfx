/*
 * Summary: Unicode character APIs
 * Description: API for the Unicode character APIs
 *
 * This file is automatically generated from the
 * UCS description files of the Unicode Character Database
 * http://www.unicode.org/Public/4.0-Update1/UCD-4.0.1.html
 * using the genUnicode.py Python script.
 *
 * Generation date: Tue Apr 30 17:30:38 2024
 * Sources: Blocks-4.0.1.txt UnicodeData-4.0.1.txt
 * Author: Daniel Veillard
 */

#ifndef __XML_UNICODE_H__
#define __XML_UNICODE_H__

#include <libxml/xmlversion.h>

#ifdef LIBXML_UNICODE_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

XML_DEPRECATED
XMLPUBFUN int xmlUCSIsAegeanNumbers    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsAlphabeticPresentationForms    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsArabic    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsArabicPresentationFormsA    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsArabicPresentationFormsB    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsArmenian    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsArrows    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsBasicLatin    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsBengali    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsBlockElements    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsBopomofo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsBopomofoExtended    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsBoxDrawing    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsBraillePatterns    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsBuhid    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsByzantineMusicalSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKCompatibility    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKCompatibilityForms    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKCompatibilityIdeographs    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKCompatibilityIdeographsSupplement    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKRadicalsSupplement    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKSymbolsandPunctuation    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKUnifiedIdeographs    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKUnifiedIdeographsExtensionA    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCJKUnifiedIdeographsExtensionB    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCherokee    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCombiningDiacriticalMarks    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCombiningDiacriticalMarksforSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCombiningHalfMarks    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCombiningMarksforSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsControlPictures    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCurrencySymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCypriotSyllabary    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCyrillic    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCyrillicSupplement    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsDeseret    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsDevanagari    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsDingbats    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsEnclosedAlphanumerics    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsEnclosedCJKLettersandMonths    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsEthiopic    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGeneralPunctuation    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGeometricShapes    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGeorgian    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGothic    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGreek    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGreekExtended    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGreekandCoptic    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGujarati    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsGurmukhi    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHalfwidthandFullwidthForms    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHangulCompatibilityJamo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHangulJamo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHangulSyllables    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHanunoo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHebrew    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHighPrivateUseSurrogates    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHighSurrogates    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsHiragana    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsIPAExtensions    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsIdeographicDescriptionCharacters    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsKanbun    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsKangxiRadicals    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsKannada    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsKatakana    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsKatakanaPhoneticExtensions    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsKhmer    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsKhmerSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLao    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLatin1Supplement    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLatinExtendedA    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLatinExtendedB    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLatinExtendedAdditional    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLetterlikeSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLimbu    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLinearBIdeograms    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLinearBSyllabary    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsLowSurrogates    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMalayalam    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMathematicalAlphanumericSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMathematicalOperators    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMiscellaneousMathematicalSymbolsA    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMiscellaneousMathematicalSymbolsB    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMiscellaneousSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMiscellaneousSymbolsandArrows    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMiscellaneousTechnical    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMongolian    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMusicalSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsMyanmar    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsNumberForms    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsOgham    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsOldItalic    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsOpticalCharacterRecognition    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsOriya    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsOsmanya    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsPhoneticExtensions    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsPrivateUse    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsPrivateUseArea    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsRunic    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsShavian    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSinhala    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSmallFormVariants    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSpacingModifierLetters    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSpecials    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSuperscriptsandSubscripts    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSupplementalArrowsA    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSupplementalArrowsB    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSupplementalMathematicalOperators    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSupplementaryPrivateUseAreaA    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSupplementaryPrivateUseAreaB    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsSyriac    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsTagalog    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsTagbanwa    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsTags    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsTaiLe    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsTaiXuanJingSymbols    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsTamil    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsTelugu    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsThaana    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsThai    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsTibetan    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsUgaritic    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsUnifiedCanadianAboriginalSyllabics    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsVariationSelectors    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsVariationSelectorsSupplement    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsYiRadicals    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsYiSyllables    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsYijingHexagramSymbols    (int code);

XMLPUBFUN int xmlUCSIsBlock    (int code, const char *block);

XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatC    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatCc    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatCf    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatCo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatCs    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatL    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatLl    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatLm    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatLo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatLt    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatLu    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatM    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatMc    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatMe    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatMn    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatN    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatNd    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatNl    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatNo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatP    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatPc    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatPd    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatPe    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatPf    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatPi    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatPo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatPs    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatS    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatSc    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatSk    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatSm    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatSo    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatZ    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatZl    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatZp    (int code);
XML_DEPRECATED
XMLPUBFUN int xmlUCSIsCatZs    (int code);

XMLPUBFUN int xmlUCSIsCat    (int code, const char *cat);

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_UNICODE_ENABLED */

#endif /* __XML_UNICODE_H__ */
