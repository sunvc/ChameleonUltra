// MAX fork hook: command table rows. Included once inside m_data_cmd_map[].
// Every fork command is registered here so app_cmd.c keeps a single hook point.
#if defined(PROJECT_CHAMELEON_ULTRA)
{    DATA_CMD_MAX_MF1_OFFLINE_CLONE,        NULL, max_cmd_offline_clone,        NULL },
{    DATA_CMD_MAX_MF1_OFFLINE_CLONE_STATUS, NULL, max_cmd_offline_clone_status, NULL },
#endif
