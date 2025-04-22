// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Rachel Mant <git@dragonmux.network>
#ifndef SNDH_ATARI_ASCII_HXX
#define SNDH_ATARI_ASCII_HXX

#include <array>
#include <string_view>

using namespace std::literals::string_view_literals;

constexpr static inline std::array<std::string_view, 256> atariChars
{{
	"\0"sv, u8"⇧"sv, u8"⇩"sv, u8"⇨"sv,
	u8"⇦"sv, u8"🮽"sv, u8"🮾"sv, u8"🮿"sv,
	u8"✓"sv, u8"🕒︎"sv, u8"🔔︎"sv, u8"♪"sv,
	"␌"sv, "␍"sv, u8"�"sv, u8"�"sv,
	u8"🯰"sv, u8"🯱"sv, u8"🯲"sv, u8"🯳"sv,
	u8"🯴"sv, u8"🯵"sv, u8"🯶"sv, u8"🯷"sv,
	u8"🯸"sv, u8"🯹"sv, u8"ə"sv, "␛"sv,
	u8"�"sv, u8"�"sv, u8"�"sv, u8"�"sv,

	" "sv, "!"sv, "\""sv, "#"sv,
	"$"sv, "%"sv, "&"sv, "'"sv,
	"("sv, ")"sv, "*"sv, "+"sv,
	","sv, "-"sv, "."sv, "/"sv,
	"0"sv, "1"sv, "2"sv, "3"sv,
	"4"sv, "5"sv, "6"sv, "7"sv,
	"8"sv, "9"sv, ":"sv, ";"sv,
	"<"sv, "="sv, ">"sv, "?"sv,
	"@"sv, "A"sv, "B"sv, "C"sv,
	"D"sv, "E"sv, "F"sv, "G"sv,
	"H"sv, "I"sv, "J"sv, "K"sv,
	"L"sv, "M"sv, "N"sv, "O"sv,
	"P"sv, "Q"sv, "R"sv, "S"sv,
	"T"sv, "U"sv, "V"sv, "W"sv,
	"X"sv, "Y"sv, "Z"sv, "["sv,
	"\\"sv, "]"sv, "^"sv, "_"sv,
	"`"sv, "a"sv, "b"sv, "c"sv,
	"d"sv, "e"sv, "f"sv, "g"sv,
	"h"sv, "i"sv, "j"sv, "k"sv,
	"l"sv, "m"sv, "n"sv, "o"sv,
	"p"sv, "q"sv, "r"sv, "s"sv,
	"t"sv, "u"sv, "v"sv, "w"sv,
	"x"sv, "y"sv, "z"sv, "{"sv,
	"|"sv, "}"sv, "~"sv, u8"△"sv,

	u8"Ç"sv, u8"ü"sv, u8"é"sv, u8"â"sv,
	u8"ä"sv, u8"à"sv, u8"å"sv, u8"ç"sv,
	u8"ê"sv, u8"ë"sv, u8"è"sv, u8"ï"sv,
	u8"î"sv, u8"ì"sv, u8"Ä"sv, u8"Å"sv,
	u8"É"sv, u8"æ"sv, u8"Æ"sv, u8"ô"sv,
	u8"ö"sv, u8"ò"sv, u8"û"sv, u8"ù"sv,
	u8"ÿ"sv, u8"Ö"sv, u8"Ü"sv, u8"¢"sv,
	u8"£"sv, u8"¥"sv, u8"ß"sv, u8"ƒ"sv,
	u8"á"sv, u8"í"sv, u8"ó"sv, u8"ú"sv,
	u8"ñ"sv, u8"Ñ"sv, u8"ª"sv, u8"º"sv,
	u8"¿"sv, u8"⌐"sv, u8"¬"sv, u8"½"sv,
	u8"¼"sv, u8"¡", u8"«"sv, u8"»"sv,
	u8"ã"sv, u8"õ"sv, u8"Ø"sv, u8"ø"sv,
	u8"œ"sv, u8"Œ"sv, u8"À"sv, u8"Ã"sv,
	u8"Õ"sv, u8"¨"sv, u8"´"sv, u8"†"sv,
	u8"¶"sv, u8"©"sv, u8"®"sv, u8"™"sv,
	u8"ĳ"sv, u8"Ĳ"sv, u8"א"sv, u8"ב"sv,
	u8"ג"sv, u8"ד"sv, u8"ה"sv, u8"ו"sv,
	u8"ז"sv, u8"ח"sv, u8"ט"sv, u8"י"sv,
	u8"כ"sv, u8"ל"sv, u8"מ"sv, u8"נ"sv,
	u8"ס"sv, u8"ע"sv, u8"פ"sv, u8"צ"sv,
	u8"ק"sv, u8"ר"sv, u8"ש"sv, u8"ת"sv,
	u8"ן"sv, u8"ך"sv, u8"ם"sv, u8"ף"sv,
	u8"ץ"sv, u8"§"sv, u8"∧"sv, u8"∞"sv,
	u8"α"sv, u8"β"sv, u8"Γ"sv, u8"π"sv,
	u8"Σ"sv, u8"σ"sv, u8"µ"sv, u8"τ"sv,
	u8"Φ"sv, u8"Θ"sv, u8"Ω"sv, u8"δ"sv,
	u8"∮"sv, u8"ϕ"sv, u8"∈"sv, u8"∩"sv,
	u8"≡"sv, u8"±"sv, u8"≥"sv, u8"≤"sv,
	u8"⌠"sv, u8"⌡"sv, u8"÷"sv, u8"≈"sv,
	u8"°"sv, u8"•"sv, u8"·"sv, u8"√"sv,
	u8"ⁿ"sv, u8"²"sv, u8"³"sv, u8"¯"sv
}};

#endif /*SNDH_ATARI_ASCII_HXX*/
