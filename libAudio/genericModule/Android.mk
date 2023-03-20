# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2017-2023 Rachel Mant <git@dragonmux.network>

MY_SRC := ModuleFile.cpp ModuleHeader.cpp ModuleInstrument.cpp ModuleSample.cpp ModulePattern.cpp ModuleEffects.cpp
LOCAL_SRC_FILES += $(addprefix genericModule/,$(MY_SRC))
