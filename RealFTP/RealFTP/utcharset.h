// =================================================================
//  class: 
//  File:  utcharset.h
//  
//  Purpose:
//
//  Various defines to help with extracting code page and charsets from
//  MIME and internet mail header encodings
//      
// ===================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ===================================================================

#ifndef UTCHARSET_H
#define UTCHARSET_H

#include <tchar.h>
#include <stdlib.h>
#include <assert.h>

typedef struct tag_CUT_CharSetStruct {
    //public:
    LPCSTR		strCharSet;
    UINT		nCodePage;
    UINT		nCharSet;
} CUT_CharSetStruct;

typedef struct tag_CUT_LocaleLangStruct {
    LPCSTR		strLocaleLang;
    LPCSTR		strCharSet;
} CUT_LocaleLangStruct;



enum {  MAX_CHARSETS = 50 };		// more than enough, for now.
                                    // init will assert if too many charsets (?)

static CUT_CharSetStruct utCharSets[MAX_CHARSETS];

class CUT_CharSet
{

public:
    CUT_CharSet(){}

    static CUT_CharSetStruct * GetCharSetStruct(LPCSTR lpszCharSet) {
        static int count = 0;
        static bool bInitDone = false;

        // return a default value? 
        CUT_CharSetStruct * retval = &utCharSets[8];

        if(!bInitDone) {
            count = InitCharSets();
            bInitDone = true;

            assert(count <= MAX_CHARSETS );
        }

        if(lpszCharSet != NULL) {
            // find in charset array...
            for( int index = 0; index <= count; ++ index) {
                if(_stricmp(lpszCharSet, utCharSets[index].strCharSet) == 0) {
                    retval = &utCharSets[index];
                    break;
                }
            }
        }
        return retval;	
    }

    // v4.2 These are intended as hints for font creation and wide char translation.
    // Compound initializers would be more convenient.

    // mapping all known RFC 1700 charsets to codepages is somewhat of a daunting task
    // so for starters lets at least map windows charset defines to codepages as given
    // in Microsoft Article ID : 165478:
    /*****************************************************
    Charset Name       Charset Value(hex)  Codepage number
    ------------------------------------------------------

    DEFAULT_CHARSET           1 (x01)
    SYMBOL_CHARSET            2 (x02)
    OEM_CHARSET             255 (xFF)
    ANSI_CHARSET              0 (x00)            1252
    RUSSIAN_CHARSET         204 (xCC)            1251
    EE_CHARSET              238 (xEE)            1250
    GREEK_CHARSET           161 (xA1)            1253
    TURKISH_CHARSET         162 (xA2)            1254
    BALTIC_CHARSET          186 (xBA)            1257
    HEBREW_CHARSET          177 (xB1)            1255
    ARABIC _CHARSET         178 (xB2)            1256
    SHIFTJIS_CHARSET        128 (x80)             932
    HANGEUL_CHARSET         129 (x81)             949
    GB2313_CHARSET          134 (x86)             936
    CHINESEBIG5_CHARSET     136 (x88)             950

    to which we might add
    JOHAB_CHARSET                                1361
    VIETNAMESE_CHARSET                           1258
    THAI_CHARSET                                  874
    *****************************************************/
    //
    // From that starting point, we add 'em as we find 'em. The CHARSET
    // defines here were initially intended as mnemonics for filling in
    // code pages. 
    //
    // The primary purpose of this array is to allow proper conversion from a 
    // multibyte 8 bit charset to unicode via MultiByteToWideChar calls. We
    // need to be able to isolate a codepage for that call based on a charset
    //.indicated in either an encoded header string, ( e.g. ?Utf-8?B?xxxxxx= )
    // or the 'charset=' component of a ContentType MIME header relating to
    // a mail or news message.
    //
    // Problems remain with malformed messages that may include raw unicode 
    // text without specifying a content type. In some cases the message body
    // may be decoded by assuming a character set specified in one of the 
    // encoded headers (most often Subject and From) can be applied to the 
    // raw body text. In some cases it must be left to the user to select an
    // encoding from a list (the explorer/CHtmlView includes such selection 
    // in its default menu).
    //
    // On the other hand, a unicode based application may naturally display
    // messages and headers that were sent as raw utf-8 characters with no
    // charset specification.
    //
    // Bear in mind that there are many more charsets and their aliases listed 
    // in RFC 1700 than are dealt with here. Our first goal is to map those 
    // that are most common in e-mail/news tranmissions, and this will eventually
    // limit itself to those used by the most commonly used mail clients.

    static int InitCharSets() 
    {
        int index = 0;
        utCharSets[index].strCharSet = "Utf-8";
        utCharSets[index].nCodePage = CP_UTF8;
        utCharSets[index].nCharSet = DEFAULT_CHARSET;		// UTF_8
        ++index;

        utCharSets[index].strCharSet = "Utf-7";
        utCharSets[index].nCodePage = CP_UTF7;
        utCharSets[index].nCharSet = DEFAULT_CHARSET;		// UTF_7
        ++index;

        utCharSets[index].strCharSet = "Windows-1250";
        utCharSets[index].nCodePage = CP_ACP;
        utCharSets[index].nCharSet = DEFAULT_CHARSET;	 
        ++index;

        utCharSets[index].strCharSet = "cp1250";
        utCharSets[index].nCodePage = CP_ACP;
        utCharSets[index].nCharSet = DEFAULT_CHARSET;		// ANSI - Central Europe
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-1";
        utCharSets[index].nCodePage = CP_ACP;
        utCharSets[index].nCharSet = DEFAULT_CHARSET;			// Latin 1 
        ++index;

        utCharSets[index].strCharSet = "Windows-1251";
        utCharSets[index].nCodePage = 1251;
        utCharSets[index].nCharSet = RUSSIAN_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "cp1251";
        utCharSets[index].nCodePage = 1251;
        utCharSets[index].nCharSet = RUSSIAN_CHARSET;		// ANSI - Cyrillic
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-5";
        utCharSets[index].nCodePage = 28595;
        utCharSets[index].nCharSet = RUSSIAN_CHARSET;			// Cyrillic
        ++index;

        utCharSets[index].strCharSet = "cp866";
        utCharSets[index].nCodePage = 866;
        utCharSets[index].nCharSet = RUSSIAN_CHARSET;			// Russian DOS
        ++index;

        utCharSets[index].strCharSet = "koi8-r";
        utCharSets[index].nCodePage = 20866;
        utCharSets[index].nCharSet = RUSSIAN_CHARSET;			// Russian
        ++index;

		utCharSets[index].strCharSet = "koi8-u";
        utCharSets[index].nCodePage = 21866;
        utCharSets[index].nCharSet = EASTEUROPE_CHARSET;		// Ukrainian
        ++index;

        utCharSets[index].strCharSet = "Windows-1252";
        utCharSets[index].nCodePage = 1252;
        utCharSets[index].nCharSet = ANSI_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "cp1252";
        utCharSets[index].nCodePage = 1252;
        utCharSets[index].nCharSet = ANSI_CHARSET;		 
        ++index;

        utCharSets[index].strCharSet = "ascii";
        utCharSets[index].nCodePage = 1252;
        utCharSets[index].nCharSet = ANSI_CHARSET;			// *** DEFAULT VALUE ***
        ++index;

        utCharSets[index].strCharSet = "us-ascii";
        utCharSets[index].nCodePage = 1252;
        utCharSets[index].nCharSet = ANSI_CHARSET;			// Latin I
        ++index;

        utCharSets[index].strCharSet = "Windows-1253";
        utCharSets[index].nCodePage = 1253;
        utCharSets[index].nCharSet = GREEK_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "cp1253";
        utCharSets[index].nCodePage = 1253;
        utCharSets[index].nCharSet = GREEK_CHARSET;			// ANSI - Greek
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-7";
        utCharSets[index].nCodePage = 28597;
        utCharSets[index].nCharSet = GREEK_CHARSET;				// Greek
        ++index;

        utCharSets[index].strCharSet = "Windows-1254";
        utCharSets[index].nCodePage = 1254;
        utCharSets[index].nCharSet = TURKISH_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "cp1254";
        utCharSets[index].nCodePage = 1254;
        utCharSets[index].nCharSet = TURKISH_CHARSET;		// ANSI - Turkish
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-3";
        utCharSets[index].nCodePage = 28593;
        utCharSets[index].nCharSet = TURKISH_CHARSET;			// Latin 3 - uncertain - Esperanto, Maltese, Turkish - replaced by Latin 5
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-10";
        utCharSets[index].nCodePage = 1254;
        utCharSets[index].nCharSet = TURKISH_CHARSET;			// Latin 6 - Innu, nordic chars
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-15";
        utCharSets[index].nCodePage = 28605;
        utCharSets[index].nCharSet = TURKISH_CHARSET;			// Latin 9 - Latin 1 with Turkish chars replacing Icelandic.
        ++index;

        utCharSets[index].strCharSet = "Windows-1255";
        utCharSets[index].nCodePage = 1255;
        utCharSets[index].nCharSet = HEBREW_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "cp1255";
        utCharSets[index].nCodePage = 1255;
        utCharSets[index].nCharSet = HEBREW_CHARSET;		// ANSI - Hebrew
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-8";
        utCharSets[index].nCodePage = 28598;
        utCharSets[index].nCharSet = HEBREW_CHARSET;			// Hebrew
        ++index;

        utCharSets[index].strCharSet = "Windows-1256";
        utCharSets[index].nCodePage = 1256;
        utCharSets[index].nCharSet = ARABIC_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "cp1256";
        utCharSets[index].nCodePage = 1256;
        utCharSets[index].nCharSet = ARABIC_CHARSET;		// ANSI - Arabic
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-6";
        utCharSets[index].nCodePage = 28596;
        utCharSets[index].nCharSet = ARABIC_CHARSET;			// Arabic
        ++index;

        utCharSets[index].strCharSet = "Windows-1257";
        utCharSets[index].nCodePage = 1257;
        utCharSets[index].nCharSet = BALTIC_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "cp1257";
        utCharSets[index].nCodePage = 1257;
        utCharSets[index].nCharSet = BALTIC_CHARSET;			// ANSI - Baltic
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-4";
        utCharSets[index].nCodePage = 28594;
        utCharSets[index].nCharSet = BALTIC_CHARSET;			// Baltic
        ++index;

        utCharSets[index].strCharSet = "Windows-1258";
        utCharSets[index].nCodePage = 1258;
        utCharSets[index].nCharSet = VIETNAMESE_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "cp1258";
        utCharSets[index].nCodePage = 1258;
        utCharSets[index].nCharSet = VIETNAMESE_CHARSET;		// ANSI - Vietnamese
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-2";
        utCharSets[index].nCodePage = 28592;
        utCharSets[index].nCharSet = EASTEUROPE_CHARSET;		// Central and East European charset
        ++index;

        utCharSets[index].strCharSet = "ISO-8859-9";
        utCharSets[index].nCodePage = 28599;
        utCharSets[index].nCharSet = EASTEUROPE_CHARSET;		// Latin 5 
        ++index;

        utCharSets[index].strCharSet = "ks_c_5601-1987";
        utCharSets[index].nCodePage = 949;
        utCharSets[index].nCharSet = HANGUL_CHARSET;	
        ++index;

        utCharSets[index].strCharSet = "korean";
        utCharSets[index].nCodePage = 1361;
        utCharSets[index].nCharSet = JOHAB_CHARSET;				// Korean 
        ++index;

        utCharSets[index].strCharSet = "EUC-KR";
        utCharSets[index].nCodePage = 1361;
        utCharSets[index].nCharSet = JOHAB_CHARSET;				// EUC - Korean
        ++index;

        utCharSets[index].strCharSet = "ISO-2022-KR";
        utCharSets[index].nCodePage = 50225;					// JOHAB_CHARSET probably invalid here
        utCharSets[index].nCharSet = JOHAB_CHARSET;				// ISO-2022-KR - Korean
        ++index;

        utCharSets[index].strCharSet = "BIG5";
        utCharSets[index].nCodePage = 950;
        utCharSets[index].nCharSet = CHINESEBIG5_CHARSET;		// Traditional Chinese Windows
        ++index;

        utCharSets[index].strCharSet = "GB2312";
        utCharSets[index].nCodePage = 936;
        utCharSets[index].nCharSet = GB2312_CHARSET;
        ++index;

        utCharSets[index].strCharSet = "chinese";
        utCharSets[index].nCodePage = 936;
        utCharSets[index].nCharSet = GB2312_CHARSET;			// Simplified Chinese
        ++index;

        utCharSets[index].strCharSet = "HZ-GB-2312";
        utCharSets[index].nCodePage = 52936;
        utCharSets[index].nCharSet = GB2312_CHARSET;			// Simplified Chinese HZ
        ++index;

        utCharSets[index].strCharSet = "shift_jis";
        utCharSets[index].nCodePage = 932;
        utCharSets[index].nCharSet = SHIFTJIS_CHARSET;		// Japanese Windows
        ++index;

        utCharSets[index].strCharSet = "EUC-JP";
        utCharSets[index].nCodePage = 20932;
        utCharSets[index].nCharSet = SHIFTJIS_CHARSET;			// EUC - Japanese
        ++index;

        utCharSets[index].strCharSet = "ISO-2022-JP";
        utCharSets[index].nCodePage = 50220;					// note also 50221, 50222
        utCharSets[index].nCharSet = SHIFTJIS_CHARSET;			// ISO-2022-JP - Japanese
        ++index;

        utCharSets[index].strCharSet = "X-EUC-TW";
        utCharSets[index].nCodePage = 874;
        utCharSets[index].nCharSet = THAI_CHARSET;				// EUC - Traditional Chinese (Taiwan)(?)

        return index;
    }

    ///////////////////////////////////////////////////////////////////////////
    // A second goal is to map windows locale IDs to registered RFC 1700 charset 
    // names so that strings can be encoded for transmission based on the current
    // locale id.
    //
    // Accepts both hex and dec string representations of the LCID, or, if strLocale
    // is NULL, will use the LCID DWORD as locale ID.
    //
    // Returns charset string as char*.  Using 'cp' type aliases (e.g. cp1250)
    ///////////////////////////////////////////////////////////////////////////
    static LPCSTR GetCharSetStringFromLocaleID(LPCTSTR strLocale, LCID localeID = 0) 
    {
        LPCSTR strReturnVal = NULL;
        DWORD lcid = 0;

        if(strLocale == NULL) {
            lcid = localeID;
        }
        else {
            if(*strLocale == _T('\0')) {			
                lcid = _tcstol(strLocale, NULL, 16);	// translate hex string
            }
            else {
                lcid = _tcstol(strLocale, NULL, 10);	// translate dec string
            }
        }
        /*
        Locale ID hex, [dec], Language string, Abbrv lang string, 
        default code page, default ansi code page
        */

        switch (lcid) {
            case 1050:			//0000041a, [1050], Croatian HRV 852 1250
            case 1051:			//0000041b, [1051], Slovak SKY 852 1250
            case 1052:			//0000041c, [1052], Albanian SQI 852 1250
            case 4122:			//0000101a, [4122], Croatian (Bosnia and Herzegovina) HRB 852 1250
            case 5146:			//0000141a, [5146], Bosnian (Latin, Bosnia and Herzegovina) BSB 852 1250
            case 6170:			//0000181a, [6170], Serbian (Latin, Bosnia and Herzegovina) SRS 852 1250
            case 1038:			//0000040e, [1038], Hungarian HUN 852 1250
            case 1045:			//00000415, [1045], Polish PLK 852 1250
            case 1029:			//00000405, [1029], Czech CSY 852 1250
            case 1048:			//00000418, [1048], Romanian ROM 852 1250
            case 1060:			//00000424, [1060], Slovenian SLV 852 1250
            case 2074:			//0000081a, [2074], Serbian (Latin) SRL 852 1250
				{
				strReturnVal = "cp1250";
				break;
				}
            case 1026:			//00000402, [1026], Bulgarian BGR 866 1251
            case 1049:			//00000419, [1049], Russian RUS 866 1251
            case 1058:			//00000422, [1058], Ukrainian UKR 866 1251
            case 1059:			//00000423, [1059], Belarusian BEL 866 1251
            case 1071:			//0000042f, [1071], FYRO Macedonian MKI 866 1251
            case 1087:			//0000043f, [1087], Kazakh KKZ 866 1251
            case 1088:			//00000440, [1088], Kyrgyz (Cyrillic) KYR 866 1251
            case 1092:			//00000444, [1092], Tatar TTT 866 1251
            case 1104:			//00000450, [1104], Mongolian (Cyrillic) MON 866 1251
            case 2092:			//0000082c, [2092], Azeri (Cyrillic) AZE 866 1251
            case 2115:			//00000843, [2115], Uzbek (Cyrillic) UZB 866 1251
            case 3098:			//00000c1a, [3098], Serbian (Cyrillic) SRB 855 1251
            case 7194:			//00001c1a, [7194], Serbian (Cyrillic, Bosnia and Herzegovina) SRN 855 1251
				{
				strReturnVal = "cp1251";
				break;
				}
            case 1027:			//00000403, [1027], Catalan CAT 850 1252
            case 1030:			//00000406, [1030], Danish DAN 850 1252
            case 1031:			//00000407, [1031], German (Germany) DEU 850 1252
            case 1033:			//00000409, [1033], English (United States) ENU 437 1252
            case 1034:			//0000040a, [1034], Spanish (Traditional Sort) ESP 850 1252
            case 1035:			//0000040b, [1035], Finnish FIN 850 1252
            case 1036:			//0000040c, [1036], French (France) FRA 850 1252
            case 1039:			//0000040f, [1039], Icelandic ISL 850 1252
            case 1040:			//00000410, [1040], Italian (Italy) ITA 850 1252
            case 1043:			//00000413, [1043], Dutch (Netherlands) NLD 850 1252
            case 1044:			//00000414, [1044], Norwegian (Bokmal) NOR 850 1252
            case 1046:			//00000416, [1046], Portuguese (Brazil) PTB 850 1252
            case 1053:			//0000041d, [1053], Swedish SVE 850 1252
            case 1057:			//00000421, [1057], Indonesian IND 850 1252
            case 1078:			//00000436, [1078], Afrikaans AFK 850 1252
            case 1080:			//00000438, [1080], Faeroese FOS 850 1252
            case 1086:			//0000043e, [1086], Malay (Malaysia) MSL 850 1252
            case 1089:			//00000441, [1089], Swahili SWK 437 1252
            case 1110:			//00000456, [1110], Galician GLC 850 1252
            case 2055:			//00000807, [2055], German (Switzerland) DES 850 1252
            case 2057:			//00000809, [2057], English (United Kingdom) ENG 850 1252
            case 2058:			//0000080a, [2058], Spanish (Mexico) ESM 850 1252
            case 2060:			//0000080c, [2060], French (Belgium) FRB 850 1252
            case 2064:			//00000810, [2064], Italian (Switzerland) ITS 850 1252
            case 2067:			//00000813, [2067], Dutch (Belgium) NLB 850 1252
            case 2068:			//00000814, [2068], Norwegian (Nynorsk) NON 850 1252
            case 2070:			//00000816, [2070], Portuguese (Portugal) PTG 850 1252
            case 2077:			//0000081d, [2077], Swedish (Finland) SVF 850 1252
            case 2110:			//0000083e, [2110], Malay (Brunei Darussalam) MSB 850 1252
            case 3079:			//00000c07, [3079], German (Austria) DEA 850 1252
            case 3081:			//00000c09, [3081], English (Australia) ENA 850 1252
            case 3082:			//00000c0a, [3082], Spanish (International Sort) ESN 850 1252
            case 3084:			//00000c0c, [3084], French (Canada) FRC 850 1252
            case 4103:			//00001007, [4103], German (Luxembourg) DEL 850 1252
            case 4105:			//00001009, [4105], English (Canada) ENC 850 1252
            case 4106:			//0000100a, [4106], Spanish (Guatemala) ESG 850 1252
            case 4108:			//0000100c, [4108], French (Switzerland) FRS 850 1252
            case 5127:			//00001407, [5127], German (Liechtenstein) DEC 850 1252
            case 5129:			//00001409, [5129], English (New Zealand) ENZ 850 1252
            case 5130:			//0000140a, [5130], Spanish (Costa Rica) ESC 850 1252
            case 5132:			//0000140c, [5132], French (Luxembourg) FRL 850 1252
            case 6153:			//00001809, [6153], English (Ireland) ENI 850 1252
            case 6154:			//0000180a, [6154], Spanish (Panama) ESA 850 1252
            case 6156:			//0000180c, [6156], French (Monaco) FRM 850 1252
            case 7177:			//00001c09, [7177], English (South Africa) ENS 437 1252
            case 7178:			//00001c0a, [7178], Spanish (Dominican Republic) ESD 850 1252
            case 8201:			//00002009, [8201], English (Jamaica) ENJ 850 1252
            case 8202:			//0000200a, [8202], Spanish (Venezuela) ESV 850 1252
            case 9225:			//00002409, [9225], English (Caribbean) ENB 850 1252
            case 9226:			//0000240a, [9226], Spanish (Colombia) ESO 850 1252
            case 10249:			//00002809, [10249], English (Belize) ENL 850 1252
            case 10250:			//0000280a, [10250], Spanish (Peru) ESR 850 1252
            case 11273:			//00002c09, [11273], English (Trinidad) ENT 850 1252
            case 11274:			//00002c0a, [11274], Spanish (Argentina) ESS 850 1252
            case 12297:			//00003009, [12297], English (Zimbabwe) ENW 437 1252
            case 12298:			//0000300a, [12298], Spanish (Ecuador) ESF 850 1252
            case 13321:			//00003409, [13321], English (Philippines) ENP 437 1252
            case 13322:			//0000340a, [13322], Spanish (Chile) ESL 850 1252
            case 14346:			//0000380a, [14346], Spanish (Uruguay) ESY 850 1252
            case 15370:			//00003c0a, [15370], Spanish (Paraguay) ESZ 850 1252
            case 16394:			//0000400a, [16394], Spanish (Bolivia) ESB 850 1252
            case 17418:			//0000440a, [17418], Spanish (El Salvador) ESE 850 1252
            case 18442:			//0000480a, [18442], Spanish (Honduras) ESH 850 1252
            case 19466:			//00004c0a, [19466], Spanish (Nicaragua) ESI 850 1252
            case 20490:			//0000500a, [20490], Spanish (Puerto Rico) ESU 850 1252
            case 1106:			//00000452, [1106], Welsh CYM 850 1252
            case 1131:			//0000046b, [1131], Quechua (Bolivia) QUB 850 1252
            case 2155:			//0000086b, [2155], Quechua (Ecuador) QUE 850 1252
            case 3179:			//00000c6b, [3179], Quechua (Peru) QUP 850 1252
            case 1074:			//00000432, [1074], Tswana TSN 850 1252
            case 1076:			//00000434, [1076], Xhosa XHO 850 1252
            case 1077:			//00000435, [1077], Zulu ZUL 850 1252
            case 1132:			//0000046c, [1132], Northern Sotho NSO 850 1252
            case 1083:			//0000043b, [1083], Sami, Northern (Norway) SME 850 1252
            case 2107:			//0000083b, [2107], Sami, Northern (Sweden) SMF 850 1252
            case 3131:			//00000c3b, [3131], Sami, Northern (Finland) SMG 850 1252
            case 4155:			//0000103b, [4155], Sami, Lule (Norway) SMJ 850 1252
            case 5179:			//0000143b, [5179], Sami, Lule (Sweden) SMK 850 1252
            case 6203:			//0000183b, [6203], Sami, Southern (Norway) SMA 850 1252
            case 7227:			//00001c3b, [7227], Sami, Southern (Sweden) SMB 850 1252
            case 8251:			//0000203b, [8251], Sami, Skolt (Finland) SMS 850 1252
            case 9275:			//0000243b, [9275], Sami, Inari (Finland) SMN 850 1252
				{
				strReturnVal = "cp1252";
				break;
				}
            case 1032:			//00000408, [1032], Greek ELL 737 1253
				{
				strReturnVal = "cp1253";
				break;
				}
            case 1055:			//0000041f, [1055], Turkish TRK 857 1254
            case 1068:			//0000042c, [1068], Azeri (Latin) AZE 857 1254
            case 1091:			//00000443, [1091], Uzbek (Latin) UZB 857 1254
				{
				strReturnVal = "cp1254";
				break;
				}
            case 1037:			//0000040d, [1037], Hebrew HEB 862 1255
				{
				strReturnVal = "cp1255";
				break;
				}
            case 1025:			//00000401, [1025], Arabic (Saudi Arabia) ARA 720 1256
            case 1056:			//00000420, [1056], Urdu URD 720 1256
            case 1065:			//00000429, [1065], Farsi FAR 720 1256
            case 2049:			//00000801, [2049], Arabic (Iraq) ARI 720 1256
            case 3073:			//00000c01, [3073], Arabic (Egypt) ARE 720 1256
            case 4097:			//00001001, [4097], Arabic (Libya) ARL 720 1256
            case 5121:			//00001401, [5121], Arabic (Algeria) ARG 720 1256
            case 6145:			//00001801, [6145], Arabic (Morocco) ARM 720 1256
            case 7169:			//00001c01, [7169], Arabic (Tunisia) ART 720 1256
            case 8193:			//00002001, [8193], Arabic (Oman) ARO 720 1256
            case 9217:			//00002401, [9217], Arabic (Yemen) ARY 720 1256
            case 10241:			//00002801, [10241], Arabic (Syria) ARS 720 1256
            case 11265:			//00002c01, [11265], Arabic (Jordan) ARJ 720 1256
            case 12289:			//00003001, [12289], Arabic (Lebanon) ARB 720 1256
            case 13313:			//00003401, [13313], Arabic (Kuwait) ARK 720 1256
            case 14337:			//00003801, [14337], Arabic (U.A.E.) ARU 720 1256
            case 15361:			//00003c01, [15361], Arabic (Bahrain) ARH 720 1256
            case 16385:			//00004001, [16385], Arabic (Qatar) ARQ 720 1256
				{
				strReturnVal = "cp1256";
				break;
				}
            case 1061:			//00000425, [1061], Estonian ETI 775 1257
            case 1062:			//00000426, [1062], Latvian LVI 775 1257
            case 1063:			//00000427, [1063], Lithuanian LTH 775 1257
				{
				strReturnVal = "cp1257";
				break;
				}
            case 1066:			//0000042a, [1066], Vietnamese VIT 1258 1258
				{
				strReturnVal = "cp1258";
				break;
				}
            case 1028:			//00000404, [1028], Chinese (Taiwan) CHT 950 950
            case 3076:			//00000c04, [3076], Chinese (Hong Kong S.A.R.) ZHH 950 950
            case 5124:			//00001404, [5124], Chinese (Macau S.A.R.) ZHM 950 950
				{
				strReturnVal = "cp950";
				break;
				}
            case 2052:			//00000804, [2052], Chinese (PRC) CHS 936 936
            case 4100:			//00001004, [4100], Chinese (Singapore) ZHI 936 936
				{
				strReturnVal = "cp936";
				break;
				}
            case 1041:			//00000411, [1041], Japanese JPN 932 932
				{
				strReturnVal = "cp932";
				break;
				}
            case 1042:			//00000412, [1042], Korean KOR 949 949
				{
				strReturnVal = "cp949";
				break;
				}
            case 1054:			//0000041e, [1054], Thai THA 874 874
				{
				strReturnVal = "cp874";
				break;
				}
            case 1067:			//0000042b, [1067], Armenian HYE 1 0
            case 1079:			//00000437, [1079], Georgian KAT 1 0
            case 1081:			//00000439, [1081], Hindi HIN 1 0
            case 1094:			//00000446, [1094], Punjabi PAN 1 0
            case 1095:			//00000447, [1095], Gujarati GUJ 1 0
            case 1097:			//00000449, [1097], Tamil TAM 1 0
            case 1098:			//0000044a, [1098], Telugu TEL 1 0
            case 1099:			//0000044b, [1099], Kannada KAN 1 0
            case 1102:			//0000044e, [1102], Marathi MAR 1 0
            case 1103:			//0000044f, [1103], Sanskrit SAN 1 0
            case 1111:			//00000457, [1111], Konkani KNK 1 0
            case 1114:			//0000045a, [1114], Syriac SYR 1 0
            case 1125:			//00000465, [1125], Divehi DIV 1 0
            case 1153:			//00000481, [1153], Maori MRI 1 0
            case 1082:			//0000043a, [1082], Maltese MLT 1 0
            case 1093:			//00000445, [1093], Bengali (India) BNG 1 0
            case 1100:			//0000044c, [1100], Malayalam (India) MYM 1 0
			default:
				{
				strReturnVal = "us-ascii";		// CP_OEMCP = 1, CP_ACP = 0
				break;
				}
        }




        return strReturnVal;
    }


};	// class CUT_CharSet


#endif // UTCHARSET_H

