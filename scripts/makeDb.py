#!/bin/env python
import os, sys, re
from xml.dom.minidom import parseString
from optparse import OptionParser

# parse args
parser = OptionParser("""%prog <xmlFile> <templateFile>
This script parses a GenICam xml file and creates an EPICS database template""")
parser.add_option("", "--devInt64",
                  action="store_true", dest="devInt64", default=False,
                  help="use int64in and int64out records. Requires at least EPICS base 3.16.1 or EPICS 7.")
options, args = parser.parse_args()
if len(args) != 2:
    parser.error("Incorrect number of arguments")
if (options.devInt64):
  GCIntegerInputRecordType = "int64in"
  GCIntegerOutputRecordType = "int64out"
else:
  GCIntegerInputRecordType = "ai"
  GCIntegerOutputRecordType = "ao"

# Check the first two lines of the feature xml file to see if arv-tool left
# the camera id there, thus creating an unparsable file
# Throw it away if it doesn't look like valid xml
# A valid first line of an xml file will be optional whitespace followed by '<'
genicam_lines = open(args[0]).readlines()
try:
    start_line = min(i for i in range(2) if genicam_lines[i].lstrip().startswith("<"))
except:
    print("Neither of these lines looks like valid XML:")
    print("".join(genicam_lines[:2]))
    sys.exit(1)

# parse xml file to dom object
xml_root = parseString("".join(genicam_lines[start_line:]).lstrip())
db_filename = args[1]

# function to read element children of a node
def elements(node):
    return [n for n in node.childNodes if n.nodeType == n.ELEMENT_NODE]  

# a function to read the text children of a node
def getText(node):
    return ''.join([n.data for n in node.childNodes if n.nodeType == n.TEXT_NODE])

# node lookup from nodeName -> node
lookup = {}
# lookup from nodeName -> recordName
records = {}
categories = []

# function to create a lookup table of nodes
def handle_node(node):
    if node.nodeName == "Group":
        for n in elements(node):
            handle_node(n)
    elif node.hasAttribute("Name"):
        name = str(node.getAttribute("Name"))
        lookup[name] = node
        # Add a leading GC_ to the name to prevent identical record names to those in ADBase.template
        recordName = "GC_" + name
        if len(recordName) > 20:
            words=re.findall('[a-zA-Z][^A-Z]*', recordName)
            for i in range(len(words)):
                word = words[i]
                if (len(word) > 3):
                    word = word[:3]
                    words[i] = word
                    s = ''
                    recordName = s.join(words)
                    if (len(recordName) <= 20): break
        if len(recordName) > 20:                    
            recordName = recordName[:20]
        i = 0
        while recordName in records.values():
            recordName = recordName[:-len(str(i))] + str(i)
            i += 1
        records[name] = recordName
        if node.nodeName == "Category":
            categories.append(name)
    elif node.nodeName != "StructReg":
        print("Node has no Name attribute", node)

# list of all nodes    
for node in elements(elements(xml_root)[0]):
    handle_node(node)

# Now make structure, [(title, [features...]), ...]
structure = []
doneNodes = []
def handle_category(category):
    # making flat structure, so if its already there then don't do anything
    if category in [x[0] for x in structure]:
        return
    node = lookup[category]
    # for each child feature of this node
    features = []
    cgs = []
    for feature in elements(node):        
        if feature.nodeName == "pFeature":
            featureName = str(getText(feature))
            featureNode = lookup[featureName]
            if str(featureNode.nodeName) == "Category":
                cgs.append(featureName)
            else:
                if featureNode not in doneNodes:
                    features.append(featureNode)   
                    doneNodes.append(featureNode)
    if features:
        if len(features) > 32:
            i = 1
            while features:
                structure.append((category+str(i), features[:32]))
                i += 1
                features = features[32:]
        else:            
            structure.append((category, features))
    for category in cgs:
        handle_category(category)

for category in categories:
    handle_category(category)

def is_node_readonly(node):
    ro = False
    referenced_node_name = ''
    for n in elements(node):
        if str(n.nodeName) in ["AccessMode", "ImposedAccessMode"]:
            ro = (getText(n) == "RO")
            break
        elif str(n.nodeName) == "pValue":
            referenced_node_name = getText(n)
    else:
        referenced_node = lookup.get(referenced_node_name)
        if referenced_node:
            ro = is_node_readonly(referenced_node)
        # SwissKnife performs a once-way conversion
        elif node.nodeName in ["SwissKnife", "IntSwissKnife"]:
            ro = True
    return ro

# Spit out a database file
db_file = open(db_filename, "w")
stdout = sys.stdout
sys.stdout = db_file

# print(a header
print('# Macros:')
print('#% macro, P, Device Prefix')
print('#% macro, R, Device Suffix')
print('#% macro, PORT, Asyn Port name')
print('#% macro, TIMEOUT, Timeout, default=1')
print('#% macro, ADDR, Asyn Port address, default=0')
print()

# for each node
for node in doneNodes:
    nodeName = str(node.getAttribute("Name"))
    ro = is_node_readonly(node)
    if node.nodeName in ["Integer", "IntReg", "IntConverter", "IntSwissKnife"]:
        print('record(%s, "$(P)$(R)%s_RBV") {' % (GCIntegerInputRecordType, records[nodeName]))
        print('  field(DTYP, "asynInt64")')
        print('  field(INP,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_I_%s")' % nodeName)
        print('  field(SCAN, "I/O Intr")')
        print('  field(DISA, "0")')
        print('}')
        print()
        if ro:
            continue        
        print('record(%s, "$(P)$(R)%s") {' % (GCIntegerOutputRecordType, records[nodeName]))
        print('  field(DTYP, "asynInt64")')
        print('  field(OUT,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_I_%s")' % nodeName)
        print('  field(DISA, "0")')
        print('}')
        print()
    elif node.nodeName in ["Boolean"]:
        print('record(bi, "$(P)$(R)%s_RBV") {' % records[nodeName])
        print('  field(DTYP, "asynInt32")')
        print('  field(INP,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_B_%s")' % nodeName)
        print('  field(SCAN, "I/O Intr")')
        print('  field(ZNAM, "No")')
        print('  field(ONAM, "Yes")'                        )
        print('  field(DISA, "0")')
        print('}')
        print()
        if ro:
            continue        
        print('record(bo, "$(P)$(R)%s") {' % records[nodeName])
        print('  field(DTYP, "asynInt32")')
        print('  field(OUT,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_B_%s")' % nodeName)
        print('  field(ZNAM, "No")')
        print('  field(ONAM, "Yes")'                                )
        print('  field(DISA, "0")')
        print('}')
        print()
    elif node.nodeName in ["Float", "Converter", "SwissKnife"]:
        print('record(ai, "$(P)$(R)%s_RBV") {' % records[nodeName])
        print('  field(DTYP, "asynFloat64")')
        print('  field(INP,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_D_%s")' % nodeName)
        print('  field(PREC, "3")'        )
        print('  field(SCAN, "I/O Intr")')
        print('  field(DISA, "0")')
        print('}')
        print()
        if ro:
            continue    
        print('record(ao, "$(P)$(R)%s") {' % records[nodeName])
        print('  field(DTYP, "asynFloat64")')
        print('  field(OUT,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_D_%s")' % nodeName)
        print('  field(PREC, "3")')
        print('  field(DISA, "0")')
        print('}')
        print()
    elif node.nodeName in ["StringReg", "String"]:
        print('record(stringin, "$(P)$(R)%s_RBV") {' % records[nodeName])
        print('  field(DTYP, "asynOctetRead")')
        print('  field(INP,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_S_%s")' % nodeName)
        print('  field(SCAN, "I/O Intr")')
        print('  field(DISA, "0")')
        print('}')
        print()
        if ro:
            continue
        print('record(stringout, "$(P)$(R)%s") {' % records[nodeName])
        print('  field(DTYP, "asynOctetWrite")')
        print('  field(OUT,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_S_%s")' % nodeName)
        print('  field(DISA, "0")')
        print('}')
        print()
    elif node.nodeName in ["Command"]:
        print('record(longout, "$(P)$(R)%s") {' % records[nodeName])
        print('  field(DTYP, "asynInt32")')
        print('  field(OUT,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_C_%s")' % nodeName)
        print('  field(DISA, "0")')
        print('}')
        print()
    elif node.nodeName in ["Enumeration"]:
        enumerations = ""
        i = 0
        defaultVal = "0"
        epicsId = ["ZR", "ON", "TW", "TH", "FR", "FV", "SX", "SV", "EI", "NI", "TE", "EL", "TV", "TT", "FT", "FF"]
        for n in elements(node):
            if str(n.nodeName) == "EnumEntry":
                if i >= len(epicsId):
                    print("More than 16 enum entries for %s mbbi record, discarding additional options." % nodeName, file=sys.stderr)
                    print("   If needed, edit the Enumeration tag for %s to select the 16 you want." % nodeName, file=sys.stderr)
                    break
                name = str(n.getAttribute("Name"))
                enumerations += '  field(%sST, "%s")\n' %(epicsId[i], name[:16])
                value = [x for x in elements(n) if str(x.nodeName) == "Value"]
                assert value, "EnumEntry %s in node %s doesn't have a value" %(name, nodeName)                
                if i == 0:
                    defaultVal = getText(value[0])
                enumerations += '  field(%sVL, "%s")\n' %(epicsId[i], getText(value[0]))
                i += 1                
        print('record(mbbi, "$(P)$(R)%s_RBV") {' % records[nodeName])
        print('  field(DTYP, "asynInt32")')
        print('  field(INP,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_E_%s")' % nodeName)
        print(enumerations, end="")
        print('  field(SCAN, "I/O Intr")')
        print('  field(DISA, "0")')
        print('}')
        print()
        if ro:
            continue        
        print('record(mbbo, "$(P)$(R)%s") {' % records[nodeName])
        print('  field(DTYP, "asynInt32")')
        print('  field(OUT,  "@asyn($(PORT),$(ADDR=0),$(TIMEOUT=1))GC_E_%s")' % nodeName)
        print('  field(DOL,  "%s")' % defaultVal)
        print(enumerations, end="")
        print('  field(DISA, "0")')
        print('}')
        print()
    else:
        print("Don't know what to do with %s" % node.nodeName, file=sys.stderr)
    
# tidy up
db_file.close()     
sys.stdout = stdout

#endObjectProperties""" % globals() )

