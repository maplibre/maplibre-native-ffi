#include <CoreFoundation/CoreFoundation.h>
#include <mln/util/i18n.hpp>

namespace mln::util::i18n {

bool isDigit(char16_t chr) {
  return CFCharacterSetIsCharacterMember(
    CFCharacterSetGetPredefined(kCFCharacterSetDecimalDigit), chr
  );
}

bool isUppercase(char16_t chr) {
  // CoreFoundation includes titlecase letters; Native requires category Lu.
  return CFCharacterSetIsCharacterMember(
           CFCharacterSetGetPredefined(kCFCharacterSetUppercaseLetter), chr
         ) &&
         !CFCharacterSetIsCharacterMember(
           CFCharacterSetGetPredefined(kCFCharacterSetCapitalizedLetter), chr
         );
}

bool isPunctuationOrSymbol(char16_t chr) {
  return CFCharacterSetIsCharacterMember(
           CFCharacterSetGetPredefined(kCFCharacterSetPunctuation), chr
         ) ||
         CFCharacterSetIsCharacterMember(
           CFCharacterSetGetPredefined(kCFCharacterSetSymbol), chr
         );
}

}  // namespace mln::util::i18n
