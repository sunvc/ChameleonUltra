// BRAVO fork hook: command table rows. Included once inside m_data_cmd_map[].
// Every fork command is registered here so app_cmd.c keeps a single hook point.
#if defined(PROJECT_CHAMELEON_ULTRA)
{    DATA_CMD_BRAVO_MF1_OFFLINE_CLONE,        NULL, bravo_cmd_offline_clone,        NULL },
{    DATA_CMD_BRAVO_MF1_OFFLINE_CLONE_STATUS, NULL, bravo_cmd_offline_clone_status, NULL },
#endif
{    6038, NULL, bravo_cmd_get_auto_polling,       NULL },
{    6039, NULL, bravo_cmd_set_auto_polling,       NULL },
{    6040, NULL, bravo_cmd_get_auto_polling_delay, NULL },
{    6041, NULL, bravo_cmd_set_auto_polling_delay, NULL },
