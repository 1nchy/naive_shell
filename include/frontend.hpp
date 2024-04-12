#ifndef _ASP_FRONTEND_HPP_
#define _ASP_FRONTEND_HPP_

#include "virtual_frontend.hpp"

#include <unordered_map>
#include <unordered_set>

namespace asp {

class shell_frontend;

class shell_frontend : public frontend_interface {
private:
    shell_frontend(int _in = STDIN_FILENO, int _out = STDOUT_FILENO, int _err = STDERR_FILENO);
public:
    static shell_frontend& singleton(int _in = STDIN_FILENO, int _out = STDOUT_FILENO, int _err = STDERR_FILENO);
    virtual ~shell_frontend();
    bool eof() const { return _end_of_file; }
    void show_information(const std::string& _s) override;
    void show_prompt() override;
    void activate() override;
    void deactivate() override;
private:
    bool wait() override;
    bool read_line();
    void clear();
private: // special character handler void(char)
    enum struct extended_char : short {
        ec_default = 0,
        ec_lf = '\n', ec_cr = '\r',
        // ec_space = ' ', ec_exclamation = '!', ec_double_quotes = '\"',
        // ec_hashtag = '#', ec_dollar = '$', ec_percent = '%',
        // ec_ampersand = '&', ec_single_quote = '\'', ec_lparentheses = '(', ec_rparentheses = ')',
        // ec_asterisk = '*', ec_plus = '+', ec_comma = ',', ec_minus = '-', ec_dot = '.', ec_slash = '/',
        // ec_0 = '0', ec_1, ec_2, ec_3, ec_4, ec_5, ec_6, ec_7, ec_8, ec_9,
        // ec_colon = ':', ec_semicolon = ';', ec_less = '<', ec_equal = '=', ec_greater = '>', ec_question = '?', ec_at = '@',
        // ec_A = 'A', ec_B, ec_C, ec_D, ec_E, ec_F, ec_G, ec_H, ec_I, ec_J,
        // ec_K, ec_L, ec_M, ec_N, ec_O, ec_P, ec_Q, ec_R, ec_S, ec_T,
        // ec_U, ec_V, ec_W, ec_X, ec_Y, ec_Z,
        // ec_lbrackets = '[', ec_backslash = '\\', ec_rbrackets = ']', ec_xor = '^', ec_underscore = '_', ec_back_quote = '`',
        // ec_a = 'a', ec_b, ec_c, ec_d, ec_e, ec_f, ec_g, ec_h, ec_i, ec_j,
        // ec_k, ec_l, ec_m, ec_n, ec_o, ec_p, ec_q, ec_r, ec_s, ec_t,
        // ec_u, ec_v, ec_w, ec_x, ec_y, ec_z,
        // ec_lbraces = '{', ec_pipe = '|', ec_rbraces = '}', ec_tilde = '~',
        ec_up = 128, ec_down, ec_left, ec_right,
        ec_esc, ec_home, ec_end, ec_del, ec_back, ec_tab,
        ec_pgup, ec_pgdn, ec_ins, ec_ctrld, ec_eof,
    };
    #define EXT_CHAR(c) ((short)extended_char::c)
    short _M_read_character();
    using character_handler = void(shell_frontend::*)(short);
    // \\r \\n
    void enter_handler(short);
    // ctrl+c \\04; ctrl+d \\03
    void ctrld_handler(short);
    void esc_handler(short);
    void up_arrow_handler(short);
    void down_arrow_handler(short);
    void left_arrow_handler(short);
    void right_arrow_handler(short);
    void home_handler(short);
    void end_handler(short);
    void del_handler(short);
    void back_handler(short);
    void tab_handler(short);
    void default_handler(short);
    const std::unordered_map<short, character_handler> _char_handler_map = {
        {(short)extended_char::ec_lf, &shell_frontend::enter_handler},
        {(short)extended_char::ec_cr, &shell_frontend::enter_handler},
        {(short)extended_char::ec_ctrld, &shell_frontend::ctrld_handler},
        {(short)extended_char::ec_esc, &shell_frontend::esc_handler},
        {(short)extended_char::ec_up, &shell_frontend::up_arrow_handler},
        {(short)extended_char::ec_down, &shell_frontend::down_arrow_handler},
        {(short)extended_char::ec_left, &shell_frontend::left_arrow_handler},
        {(short)extended_char::ec_right, &shell_frontend::right_arrow_handler},
        {(short)extended_char::ec_home, &shell_frontend::home_handler},
        {(short)extended_char::ec_end, &shell_frontend::end_handler},
        {(short)extended_char::ec_del, &shell_frontend::del_handler},
        {(short)extended_char::ec_back, &shell_frontend::back_handler},
        {(short)extended_char::ec_tab, &shell_frontend::tab_handler},
        {(short)extended_char::ec_default, &shell_frontend::default_handler},
    };
    // special character escaping processing // todo, these should be implemented in backend
    bool char_2_escape(char) const;
    std::string escape_string(const std::string&) const;
    const std::unordered_set<char> _char_2_escape_set = {
        ' ', '\'', '\"', '\\', '`'
    };
private: // tab
    bool has_tab_next();
    bool has_tab_list();
    void switch_tab_list();
    size_t front_signature() const;
    inline bool usable_tab_list() const;
    std::string _tab_next;
    std::vector<std::string> _tab_list;
    size_t _tab_index = 0;
    size_t _tab_next_signature = 0;
    size_t _tab_list_signature = -1;
private: // view
    void cursor_move_back(size_t); // move cursor back
    void fill_blank(size_t);
    void text_backspace(size_t);
    void rewrite_front();
    void rewrite_back();
    bool empty() const { return _front.empty() && _back.empty(); }
    void build_line();
private: // termios
    void _M_cooked();
    void _M_raw();
    void _M_term_start_handler();
    void _M_term_end_handler();
public: // ansi color mode
    enum ansi_color {
        default_color = 0,
        black, red, green, yellow, blue, magenta, cyan, white,
        color_count,
    };
    static const char* color_mode[];
private: // I/O
    ssize_t _M_read(char& _c);
    ssize_t _M_nonblock_read(char& _c);
    ssize_t _M_write(char _c);
    ssize_t _M_write(const std::string& _s);
    ssize_t _M_error(char _c);
    ssize_t _M_error(const std::string& _s);
private:
    std::vector<char> _front;
    std::vector<char> _back;
    const std::string _prompt = "> ";
    bool _end_of_file = false;
    termios _shell_in_setting, _default_in_setting;
};

};

#endif // _ASP_FRONTEND_HPP_