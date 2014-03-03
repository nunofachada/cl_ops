# Summary

In progress. More information soon.

## How to show debug messages

### Show debug messages in stdout

`G_MESSAGES_DEBUG=cl_ops clo_command`

### Normal output to stdout, debug output to a file

`G_MESSAGES_DEBUG=cl_ops clo_command | tee >(grep "cl_ops-DEBUG" > debug.txt) | grep -v "cl_ops-DEBUG"`
