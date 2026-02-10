#include "../inc/clean_comments.hpp"

std::string clean_comments(std::ifstream &fs, std::string output_file, bool write_output_file )
{
    std::stringstream clean_ss;
    char c;
    bool in_multiline = false;
    bool in_singleline = false;
    bool last_char_is_special = false;
    std::string final_output;

    // Remove comments
    while (fs.get(c))
    {
        if (in_multiline)
        {
            if (c == '*' && fs.peek() == '/')
            {
                fs.get(); // remove '/'
                in_multiline = false;
            }
        }
        else if (in_singleline)
        {
            if (c == '\n')
            {
                in_singleline = false;
                clean_ss.put(c); // save '\n'
            }
        }
        else
        {
            if (c == '/' && fs.peek() == '*')
            {
                fs.get(); // remove '*'
                in_multiline = true;
            }
            else if (c == '/' && fs.peek() == '/')
            {
                fs.get(); // remove '/'
                in_singleline = true;
            }
            else
            {
                clean_ss.put(c);
            }
        }
    }

    std::string cleaned_content = clean_ss.str();
    std::string insert_n;
    std::string remove_n;
    std::string insert_space;

    // Substitute '\t' and full-width space to ' '
    // Insert '\n' after ';', '{', '}', '\', ':'
    for (size_t i = 0; i < cleaned_content.size(); ++i)
    {

        // Substitute '\t' and full-width space to ' '
        if (cleaned_content[i] == '\t')
        {
            insert_n += ' ';
        }
        else if (i + 2 < cleaned_content.size() &&
                 static_cast<unsigned char>(cleaned_content[i]) == 0xE3 &&
                 static_cast<unsigned char>(cleaned_content[i + 1]) == 0x80 &&
                 static_cast<unsigned char>(cleaned_content[i + 2]) == 0x80)
        {
            insert_n += ' ';
            i += 2;
        }
        else
        {
            insert_n += cleaned_content[i];
        }

        // Insert '\n' after ';', '{', '}', '\', ':'
        if ((cleaned_content[i] == ';' ||
             cleaned_content[i] == '\\' ||
             cleaned_content[i] == '{' ||
             cleaned_content[i] == '}') &&
            cleaned_content[i + 1] != '\n')
        {
            insert_n += '\n';
        }
    }


    bool remove_change_line = false;
    // Remove '\n' if the front char is not special
    if (remove_change_line){

        for (size_t i = 0; i < insert_n.size(); ++i)
        {
            if (insert_n[i] == '\n')
            {
                if (last_char_is_special)
                {
                    remove_n += '\n';
                }
                last_char_is_special = false;
            }
            else
            {
                remove_n += insert_n[i];
                if (insert_n[i] == ';' || insert_n[i] == '{' || insert_n[i] == '}' || insert_n[i] == '\\')
                {
                    last_char_is_special = true;
                }
                else if (insert_n[i] != ' ')
                {
                    last_char_is_special = false;
                }
            }
        }
    }
    else {
        remove_n = insert_n;
    }

    // Insert ' ' before/after ';', ':', '('
    for (size_t i = 0; i < remove_n.size(); ++i)
    {
        if (remove_n[i] == ';' ||
            remove_n[i] == '(' ||
            remove_n[i] == ')' ||
            remove_n[i] == '{' ||
            remove_n[i] == '}' ||
            remove_n[i] == ',' ||
            remove_n[i] == '\\')

        {
            insert_space += ' ';
            insert_space += remove_n[i];
            insert_space += ' ';
        }
        else
        {
            insert_space += remove_n[i];
        }
    }

    std::string remove_space;

    // remove ' ' before ' ', ')'
    for (size_t i = 0; i < insert_space.size(); ++i)
    {
        if (insert_space[i] == ' ' && insert_space[i - 1] == ' ')
        {
        }
        else
        {
            remove_space += insert_space[i];
        }
    }

    // write to output file for debug
    if (write_output_file)
    {
        std::ofstream ofs(output_file);
        if (!ofs.is_open())
        {
            throw std::runtime_error("Error: Cannot open output file: " + output_file);
        }
        ofs << remove_space;
        ofs.close();
    }
    return remove_space;
}

