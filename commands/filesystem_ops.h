#ifndef FILESYSTEM_OPS_H
#define FILESYSTEM_OPS_H

#include "../shell/parser.h"
#include "../shell/executor.h"

/* LiteShell Native Filesystem Commands */
OperationResult ops_makef(Command *cmd);
OperationResult ops_delf(Command *cmd);
OperationResult ops_copyf(Command *cmd);
OperationResult ops_movef(Command *cmd);
OperationResult ops_typef(Command *cmd);

OperationResult ops_makedir(Command *cmd);
OperationResult ops_deldir(Command *cmd);
OperationResult ops_copydir(Command *cmd);
OperationResult ops_movedir(Command *cmd);
OperationResult ops_dir(Command *cmd);

OperationResult ops_cdir(Command *cmd);
OperationResult ops_curdir(Command *cmd);

/* Aliases and Backward Compatibility Handlers */
OperationResult ops_cd(Command *cmd);
OperationResult ops_pwd(Command *cmd);
OperationResult ops_mkdir(Command *cmd);
OperationResult ops_rmdir(Command *cmd);
OperationResult ops_copy(Command *cmd);
OperationResult ops_move(Command *cmd);
OperationResult ops_del(Command *cmd);
OperationResult ops_type(Command *cmd);

#endif
