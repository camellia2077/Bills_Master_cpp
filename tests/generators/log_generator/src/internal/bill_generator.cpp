#include "bill_generator.h"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <utility>

BillGenerator::BillGenerator(std::vector<GeneratorCategoryConfig> categories,
                             double comment_probability,
                             std::vector<std::string> comments)
    : categories_(std::move(categories)),
      comment_probability_(comment_probability),
      comments_(std::move(comments)),
      random_engine_(std::random_device{}()) {}

auto BillGenerator::generate_bill_content(int year, int month) const
    -> std::string {
  std::ostringstream output;
  output << "date:" << year << "-" << std::setw(2) << std::setfill('0')
         << month << "\n";
  output << "remark:\n\n";

  std::map<std::string, std::vector<const GeneratorCategoryConfig*>>
      grouped_by_parent;
  for (const auto& sub_item : categories_) {
    grouped_by_parent[sub_item.parent_category].push_back(&sub_item);
  }

  for (auto parent_iter = grouped_by_parent.begin();
       parent_iter != grouped_by_parent.end(); ++parent_iter) {
    const std::string& parent_name = parent_iter->first;
    const auto& sub_items = parent_iter->second;
    output << parent_name << "\n";

    for (const auto& sub_config : sub_items) {
      output << "\n" << sub_config->sub_category << "\n";

      std::vector<GeneratorDetailConfig> details_vector = sub_config->details;
      if (details_vector.empty()) {
        continue;
      }

      const int generate_count =
          random_int(1, static_cast<int>(details_vector.size()));
      std::shuffle(details_vector.begin(), details_vector.end(), random_engine_);

      for (int detail_index = 0; detail_index < generate_count; ++detail_index) {
        const auto& item = details_vector[detail_index];
        const double cost = random_double(item.min_cost, item.max_cost);
        const double adjustment = random_double(-10.0, 10.0);

        if (parent_name == "income") {
          output << "+";
        }

        const bool use_multiplication = (random_double(0.0, 1.0) < 0.3);
        if (use_multiplication) {
          const int quantity = random_int(2, 6);
          const double unit_price = cost / quantity;
          output << std::fixed << std::setprecision(2) << unit_price
                 << (random_int(0, 1) == 0 ? "*" : "×") << quantity;
        } else {
          output << std::fixed << std::setprecision(2) << cost;
        }

        output << std::showpos << std::fixed << std::setprecision(2)
               << adjustment;
        output << std::noshowpos << " " << item.description;

        if (!comments_.empty() &&
            random_double(0.0, 1.0) < comment_probability_) {
          const int comment_index =
              random_int(0, static_cast<int>(comments_.size()) - 1);
          output << " // " << comments_[comment_index];
        }

        output << "\n";
      }
    }

    if (std::next(parent_iter) != grouped_by_parent.end()) {
      output << "\n";
    }
  }

  return output.str();
}

auto BillGenerator::random_double(double min, double max) const -> double {
  std::uniform_real_distribution<> dist(min, max);
  return dist(random_engine_);
}

auto BillGenerator::random_int(int min, int max) const -> int {
  std::uniform_int_distribution<> dist(min, max);
  return dist(random_engine_);
}
