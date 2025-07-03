#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <string>

/**
 * @class AppController
 * @brief Acts as the central controller for the application.
 *
 * This class encapsulates the core business logic, such as validation,
 * modification, database import, and querying. It is designed to be called
 * by different user interfaces (e.g., an interactive menu or a command-line tool).
 */
class AppController {
public:
    AppController();

    /**
     * @brief Handles the validation of one or more bill files.
     * @param path A path to a single .txt file or a directory to search recursively.
     */
    void handle_validation(const std::string& path);

    /**
     * @brief Handles the modification of one or more bill files.
     * @param path A path to a single .txt file or a directory to search recursively.
     */
    void handle_modification(const std::string& path);

    /**
     * @brief Handles the import of one or more bill files into the database.
     * @param path A path to a single .txt file or a directory to search recursively.
     */
    void handle_import(const std::string& path);

    /**
     * @brief Handles the full processing workflow (validate -> modify -> import).
     * @param path A path to a single .txt file or a directory to search recursively.
     */
    void handle_full_workflow(const std::string& path);

    /**
     * @brief Handles querying and exporting a yearly summary report.
     * @param year The 4-digit year to query.
     */
    void handle_yearly_query(const std::string& year);

    /**
     * @brief Handles querying and exporting a monthly details report.
     * @param month The 6-digit month to query (YYYYMM).
     */
    void handle_monthly_query(const std::string& month);

    /**
     * @brief Displays the application's version information.
     */
    void display_version();
};

#endif // APP_CONTROLLER_H
