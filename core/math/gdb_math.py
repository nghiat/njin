import gdb

# Paste these code to ~/.gdbinit:
# python
# import sys
# sys.path.insert(0, "<path-to-project>/math")
# import gdb_math
# end
class vec_printer:
    """Print a nj_v*_t object."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        length = int(str(self.val.type)[3])
        a = self.val["a"]
        str_val = ""
        for i in range(0, length - 1):
            str_val += str(a[i]) + ", "
        str_val += str(a[length - 1])
        return "{" + str_val + "}"

def lookup_type(val):
    type = str(val.type)
    if type.startswith("nj_v") and type.endswith("_t") and len(type) < 8:
        return vec_printer(val)
    return None

gdb.pretty_printers.append(lookup_type)
