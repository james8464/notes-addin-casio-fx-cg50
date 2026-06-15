#ifndef KHICAS_SUITE_BRIDGE_HPP
#define KHICAS_SUITE_BRIDGE_HPP

bool suite_eval_with_working(const char *expr);
const char *suite_console_fmenu_config();
int suite_custom_fkey_label(int fkey);
void suite_register_lexer_symbols();
bool suite_is_custom_command(const char *name);

#endif
