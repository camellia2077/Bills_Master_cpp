#ifndef RECORD_TEMPLATE_RECORD_EDITOR_SERVICE_HPP_
#define RECORD_TEMPLATE_RECORD_EDITOR_SERVICE_HPP_

#include <string>
#include <string_view>

#include "record_template/record_editor_types.hpp"

class RecordEditorService {
 public:
  // Parses canonical record TXT into an editor-oriented structure that keeps
  // parent/sub-title boundaries intact for UI editing.
  [[nodiscard]] static auto Parse(std::string_view raw_text)
      -> RecordEditorResult<RecordEditorDocument>;

  // Serializes the editor structure back into normalized TXT without allowing
  // the UI layer to rewrite section titles ad hoc.
  [[nodiscard]] static auto Serialize(const RecordEditorDocument& document)
      -> RecordEditorResult<std::string>;
};

#endif  // RECORD_TEMPLATE_RECORD_EDITOR_SERVICE_HPP_
