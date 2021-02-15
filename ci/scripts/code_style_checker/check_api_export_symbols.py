#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

import re
from common_modules import common


def check_api_export_symbols(filename, clean_file_contents):
    """
    Check that public API symbols are exported
    - A class has RLOGIC_API prefix before the name, structs and enums don't have it
    """

    is_api = ("include/public" in filename)
    is_header = (filename[-2:] == ".h") or (filename[-4:] == ".hpp")
    is_api_header = is_api and is_header

    if is_api_header:
        # symbol_def_re matches following patterns:
        #   (correct case) class|struct RLOGIC_API SymbolName { [...]
        #   (correct case) class|struct RLOGIC_API SymbolName : [...]
        #   (correct case) enum|enum class SymbolName : [...]
        #   (wrong case)   class|struct SymbolName { [...]
        #   (wrong case)   class|struct SymbolName : [...]
        symbol_def_re = re.compile(r'(template [^;]+?)?\s+(enum|class|struct|enum\s+class)(\s+)(\w+)(\s+)(\w*)(\s*)(\{|\:)')

        for symbol_match in re.finditer(symbol_def_re, clean_file_contents):
            line_number = clean_file_contents[:symbol_match.start()].count("\n")

            symbolNameGroups = symbol_match.groups()
            isTemplate = symbolNameGroups[0] is not None
            isEnum = symbolNameGroups[1].strip() == "enum"
            isStruct = symbolNameGroups[1].strip() == "struct"
            isEnumClass = "enum " in symbolNameGroups[1].strip()
            firstWord = symbolNameGroups[3].strip()
            secondWord = symbolNameGroups[5].strip()

            # check special cases that should NOT have RLOGIC_API
            if isEnum:
                if firstWord == "RLOGIC_API":
                    common.log_warning("check_api_export_symbols", filename, line_number + 1, "Enum exported as RLOGIC_API: " + secondWord)
            elif isEnumClass:
                if firstWord == "RLOGIC_API":
                    common.log_warning("check_api_export_symbols", filename, line_number + 1, "Enum class exported as RLOGIC_API: " + secondWord)
            elif isStruct:
                if firstWord == "RLOGIC_API":
                    common.log_warning("check_api_export_symbols", filename, line_number + 1, "Struct exported as RLOGIC_API: " + secondWord)
            elif isTemplate:
                if firstWord == "RLOGIC_API":
                    common.log_warning("check_api_export_symbols", filename, line_number + 1, "Template exported as RLOGIC_API: " + secondWord)
            else:
                # remaining cases should have RLOGIC_API
                if firstWord != "RLOGIC_API":
                    common.log_warning("check_api_export_symbols", filename, line_number + 1, "Public symbol not exported as RLOGIC_API: " + firstWord)
    else:
        # Will find occurances of RLOGIC_API surrounded by space(s)
        RLOGIC_API_re = re.compile(r'(?<!\w)(RLOGIC_API)\s')
        for symbol_match in re.finditer(RLOGIC_API_re, clean_file_contents):
            line_number = clean_file_contents[:symbol_match.start()].count("\n")
            common.log_warning("check_api_export_symbols",
                               filename,
                               line_number + 1,
                               "Exporting API symbol in a non-API-header file! This symbol will be unusable.")
