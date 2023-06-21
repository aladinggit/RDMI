import re
import json

def parse_kgraph(line):
    global cur_ds, cur_pts, lines
    lines = []
    cur_ds = 0
    cur_pts = 0
    pattern = "kgraph\\s*\\((\\w+)\\)"
    x = re.search(pattern, line)
    if (not x):
        raise ValueError('Kgraph symbol wrong')
    cur_ds = data["entry_point"][x.group(1)] # parse the current data structure
#     return "KernelGraph(" + x.group(1) + ")"
    lines.append(kg(x.group(1)))

def parse_traverse(line):
    global cur_ds, new_ds, lines, cur_ptr
    #pattern = "\\.traverse\\s*\\((\w+.*\w+),\\s*(\\w+),\\s*(\w+)\\)"
    pattern = "\\.traverse\\s*\\((\w+.*\w+),\\s*(\\w+.*\\w+),\\s*(\w+)\\)"
    x = re.search(pattern, line)
    if (not x):
        raise ValueError('traverse symbol wrong')
    # handling the first part
    next = x.group(1).split(".")
    offset = 0
    new_ds = cur_ds
    head = 0
    for i in next:
        offset += data["data_structure"][new_ds][i]["offset"]
        new_ds = data["data_structure"][new_ds][i]["type"]
    if new_ds != x.group(3):
        head = offset
    #trans = ".traverse({},{},{})".format(offset, x.group(2), head)  
    #return trans
    #end_addr is a runtime variable
    end_addr = data["runtime_variable"][x.group(2)]
    lines.append(traverse(offset, end_addr, head))

    
def parse_iter(line):
    # needs to specially handle this
    global cur_ds, lines, cur_ptr, new_ds
    pattern_dyn = "\\.iterate\\s*\\((\\w+.*\\w+),\\s*(\\w+),\\s*(\\w+)\\)"
    pattern_fix = "\\.iterate\\s*\\((\\w+.*\\w+),\\s*(\\d+),\\s*(\\w+)\\)"
    dyn = 0
    x = re.search(pattern_fix, line)
    if (not x):
        x = re.search(pattern_dyn, line)
        dyn = 1
        if (not x):
            raise ValueError('iter symbol wrong')
    if (x.group(1) == "this"): # must be iterating array
        offset = 0
        num = x.group(2) 
        #size = x.group(3) # example for using the 3 rd 
        size = data["data_structure"][x.group(3)]["size"]
        # cur_ds and cur_ptr is not changed
        lines.append(iterate(offset, dyn, num, size))
    else:
        next_ = x.group(1).split(".")
        offset = 0
        new_ds = cur_ds
        for i in next_:
            offset += data["data_structure"][new_ds][i]["offset"]
            cur_ptr = data["data_structure"][new_ds][i]["pointer"] 
    
            if cur_ptr < 0:
                raise ValueError('ptr is minus')
            new_ds = data["data_structure"][new_ds][i]["type"] 
#         offset = data["data_structure"][cur_ds][x.group(1)]["offset"]
        num = x.group(2)
        size = data["data_structure"][x.group(3)]["size"]

#         size = x.group(3)
#         size = data["data_structure"][cur_ds][x.group(1)]["size"]  # no need to specify the size
#         cur_ds = data["data_structure"][cur_ds][x.group(1)]["type"]
        cur_ds = new_ds
#         cur_ptr = data["data_structure"][cur_ds][x.group(1)]["pointer"]
#         return ".in({})".format(offset)
        lines.append(iterate(offset, dyn, num, size))

def parse_values(line):
    # distinguish this or not this
    global cur_ds, lines, cur_ptr, new_ds
#     pattern = "\\.values\\s*\\((\\w+(,\\s*\\w+)*)\\)"
    # handle . case
    pattern = "\\.values\\s*\\((\\w+\.*\\w*(,\\s*\\w+\.*\\w*)*)\\)"
    x = re.search(pattern, line)
    if (not x):
        raise ValueError('values symbol wrong')
    # handling the first part
    offset = []
    marker = 0 # mark this
    print("group is ", x.group(1))
    for i in x.group(1).replace(" ", "").split(","):
        if i == "this":
            lines.append(values([["this", 0]], "", 0))
            marker = 1
#         off = data["data_structure"][cur_ds][i]["offset"]
#         offset.append([i, off]) # name + offset
        next_ = i.split(".")
        off = 0
        new_ds = cur_ds
        for i in next_:
            off += data["data_structure"][new_ds][i]["offset"]
            new_ds = data["data_structure"][new_ds][i]["type"] 
        offset.append([i, off]) # name + offset
    if marker == 0:
        lines.append(values(offset, "", 0))
#     print(offset)

def parse_in(line):
    # need to check on current pointer layer
    # when will in(this) happen:
    #    the current ds is a ptr(level >= 1)
    global cur_ds, cur_ptr, lines, new_ds
    pattern_in = ".in\\s*\\((\\w+.*\\w+)\\)"
    pattern_dec = ".in\\s*\\((\\w+.*\\w+),\\s*@struct\\s*(\\w+),\\s*@(\\w+)\\)"
    type_ = 0
    x = re.search(pattern_dec, line)
    type_ = 1
#     print(x.group(1))
    if (not x):
        x = re.search(pattern_in, line)
        type_ = 0
        print(type_)
        if (not x):
            raise ValueError('in symbol wrong')
    if x.group(1) == "this":  # check on special occasion
        if cur_ptr == 0: # wrong dereference
            raise ValueError('invalid dereference symbol wrong')
        else:
            cur_ptr -= 1
            if cur_ptr < 0:
                raise ValueError('in: invalid dereference symbol wrong')
            lines.append(in_(0, 0, ""))
    else: # not handling this
        if type_ == 0: # without decoration
            # handle . situation
            next_ = x.group(1).split(".")
            print("here we are !!!!!!!")
            offset = 0
            new_ds = cur_ds
            for i in next_:
                offset += data["data_structure"][new_ds][i]["offset"]
                cur_ptr = data["data_structure"][new_ds][i]["pointer"] - 1
                new_ds = data["data_structure"][new_ds][i]["type"]            
#             offset = data["data_structure"][cur_ds][x.group(1)]["offset"]
#             cur_ptr = data["data_structure"][cur_ds][x.group(1)]["pointer"] - 1 # one time dereference
#             cur_ds = data["data_structure"][cur_ds][x.group(1)]["type"]
#            return ".in({})".format(offset)
            if cur_ptr < 0:
                raise ValueError('ptr is minus')
            cur_ds = new_ds
            lines.append(in_(offset, 0, ""))
        if type_ == 1: # with decoration
            # the second will be the data structure of new cur_ds
            #offset = data["data_structure"][cur_ds][x.group(1)]["offset"]
            next_ = x.group(1).split(".")
            offset = 0
            new_ds = cur_ds
            for i in next_:
                print("new ds is ,", new_ds)
                offset += data["data_structure"][new_ds][i]["offset"]
                new_ds = data["data_structure"][new_ds][i]["type"]  
            cur_ds = x.group(2)
            new_offset = data["data_structure"][cur_ds][x.group(3)]["offset"]
            cur_ptr = 0 # semantic requirement
            #return ".in({}, @0, {})".format(offset, new_offset)
            lines.append(in_(offset, 1, new_offset))

def parse_assert(line):
    pattern = "\\.assert\\s*\\((\\w+)<\\s*(\\w+)<\\s*(\\w+)\\)"
    x = re.search(pattern, line)
    if (not x):
        raise ValueError('assert symbol wrong')
    lines.append(_assert((x.group(1))[:2]+(x.group(1))[10:], (x.group(3))[:2]+(x.group(3))[10:]))
    

class kg:
    start = ""
    type_s = "kg"
    def __init__(self, start):
        self.start = start
    def gen_code(self):
        print("gen kg code ")
        return "KernelGraph({})\n".format(self.start)

class _assert:
    low = ""
    high = ""
    type_s = "assert"
    def __init__(self, low, high):
        self.low = low
        self.high = high
    def gen_code(self):
        print("gen assert code")
        return ".assert({},{})\n".format(self.low, self.high)
    
class traverse:
    next_ptr = ""
    end = ""
    ds = ""
    type_s = "traverse"
    def __init__(self, next_ptr, end, ds):
        self.next_ptr = next_ptr
        self.end = end
        self.ds = ds
        print("next_ptr, end, ds ", self.next_ptr, self.end, self.ds)
    def gen_code(self):
        print("gen traverse code ")
        return ".traverse({}, {}, {})\n".format(str(self.next_ptr), self.end, self.ds)

class iterate:
    start = ""
    dyn = 0
    num = ""
    size = ""
    type_s = "iterate"
    def __init__(self, start, dyn, num, size):
        self.start = start
        self.dyn = dyn
        self.num = num
        self.size = size
        print("start, dyn, num, size ", self.start, self.dyn, self.num, self.size)
    def gen_code(self):
        print("gen iterate code ")
        return ".iter({}, {}, {})\n".format(str(self.start), self.num, str(self.size))
        
        
class in_:
    offset = ""
    dec = 0
    new_offset = ""
    type_s = "in"
    def __init__(self, offset, dec, new_offset):
        self.offset = offset
        self.dec = dec
        self.new_offset = new_offset
        print("offset, dec, new_offset", self.offset, self.dec, self.new_offset)
    def gen_code(self):
        print("gen in code ")
        if self.dec == 0:
            return ".in({})\n".format(self.offset)
        else:
            return ".in({}, @0, @{})\n".format(str(self.offset),  str(self.new_offset))
        
class values:
    offset = []
    name = ""
    fd_size = 0 
    type_s = "values"
    def __init__(self, offset, name, fd_size):
        self.offset = offset
        self.name = name
        self.fd_size = fd_size
        print("offset, name, fd_size", self.offset, self.name, self.fd_size)
    def gen_code(self):
        print("gen values code ")
        print(self.fd_size)
        if self.fd_size == 1: # handle special case
            return ".values(@{}, {})\n".format(self.name, str(self.offset[0][1]))
        else:
#             print(self.offset)
            code = ".values("
            for k in range(len(self.offset)-1):
                code += str(self.offset[k][1])
                code += ","
            print(self.offset)
            code += str(self.offset[len(self.offset)-1][1])
            code += ")\n"
            return code

def mark_iter():
    # to do: mark values
    global lines
    #print("begin marking iterate")
    for i in range(len(lines)):
        if lines[i].type_s == "iterate":
            if lines[i].dyn == 0:
                continue
            else:
                if i <= 0:
                    raise ValueError('iterate is placed at the first place')
                if lines[i-1].type_s == "values":
                    if lines[i-1].offset[0][0]== lines[i].num:# check on name
                        lines[i-1].fd_size = 1
                        lines[i-1].name = lines[i].num
                        print("found values")
                        continue
                elif lines[i-2].type_s == "values":
                    if lines[i-2].offset[0][0]== lines[i].num:# check on name
                        lines[i-2].fd_size = 1
                        lines[i-2].name = lines[i].num
                        print("found values")
                        continue
                else:
                    raise ValueError('cannot find the iterate dynamic entry number')
    return 1

def code_gen():
    global lines
    text = ""
    for i in lines:
        text += i.gen_code()
    return text

file1 = open('raw_policy/sample.dsl', 'r')
data = json.load(open('datastruct.json', 'r'))
dsl = file1.readlines()
lines = 0
cur_ds = 0
cur_ptr = 0
text = ""
for i in range(len(dsl)):
    if "kgraph" in dsl[i]:
#         print(dsl[i])
        parse_kgraph(dsl[i])
    elif "values" in dsl[i]:
#         print(1)
#         print(dsl[i])
        parse_values(dsl[i])
    elif "iterate" in dsl[i]:
        parse_iter(dsl[i])
    elif "traverse" in dsl[i]:
        parse_traverse(dsl[i])
    elif "in" in dsl[i]:
        parse_in(dsl[i])
    elif "assert" in dsl[i]:
        parse_assert(dsl[i])
a = mark_iter()
text = code_gen()
text += "End\n"
print(text)

