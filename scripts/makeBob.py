#!/usr/bin/env python3


import phoebusgen


import os
import sys
import re
from xml.dom.minidom import parseString
from optparse import OptionParser

import phoebusgen.screen
import phoebusgen.widget

# parse args
parser = OptionParser(
    """%prog <xmlFile> <bobFileBase>

This script parses a GenICam xml file and creates
phoebus bob screens to go with it.
The bob files will be called:
    <bobFile>-features_[1-N].bob"""
)
options, args = parser.parse_args()
if len(args) != 2:
    parser.error("Incorrect number of arguments")

# Check the first two lines of the feature xml file to see if arv-tool left
# the camera id there, thus creating an unparsable file
# Throw it away if it doesn't look like valid xml
# A valid first line of an xml file will be optional whitespace followed by '<'
genicam_lines = open(args[0]).readlines()
try:
    start_line = min(
        i for i in range(2) if genicam_lines[i].lstrip().startswith("<")
    )
except Exception as e:
    print(str(e))
    print("Neither of these lines looks like valid XML:")
    print("".join(genicam_lines[:2]))
    sys.exit(1)

# parse xml file to dom object
xml_root = parseString("".join(genicam_lines[start_line:]).lstrip())
camera_name = os.path.basename(args[1])
bobFile = args[1] + "-features_"


# function to read element children of a node
def elements(node):
    return [n for n in node.childNodes if n.nodeType == n.ELEMENT_NODE]


# a function to read the text children of a node
def getText(node):
    texts = [n.data for n in node.childNodes if n.nodeType == n.TEXT_NODE]
    return "".join(texts)


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
        # Add a leading GC_ to the name to prevent identical
        # record names to those in ADBase.template
        recordName = "GC_" + name
        if len(recordName) > 20:
            words = re.findall("[a-zA-Z][^A-Z]*", recordName)
            for i in range(len(words)):
                word = words[i]
                if len(word) > 3:
                    word = word[:3]
                    words[i] = word
                    s = ""
                    recordName = s.join(words)
                    if len(recordName) <= 20:
                        break
        if len(recordName) > 20:
            recordName = recordName[:20]
        i = 0
        while recordName in records.values():
            recordName = recordName[: -len(str(i))] + str(i)
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
                structure.append((category + str(i), features[:32]))
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
    referenced_node_name = ""
    for n in elements(node):
        if str(n.nodeName) in ["AccessMode", "ImposedAccessMode"]:
            ro = getText(n) == "RO"
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


def quoteString(string):
    escape_list = ["\\", "{", "}", '"']
    for e in escape_list:
        string = string.replace(e, "\\" + e)
    string = string.replace("\n", "").replace(",", ";")
    return string


# Write each section
stdout = sys.stdout

# Generate feature screens
maxScreenWidth = 1600
maxScreenHeight = 850
headingHeight = 20
labelWidth = 220
maxLabelHeight = 20
readonlyWidth = 240
readonlyHeight = 18
readbackWidth = 120
readbackHeight = 18
textEntryWidth = 60
textEntryHeight = 20
menuWidth = 120
menuHeight = 20
messageButtonWidth = 200
messageButtonHeight = 20
# boxWidth must be set to the widest combination of widgets in one row
boxWidth = 5 + labelWidth + 5 + menuWidth + 5 + readbackWidth + 5
w = boxWidth
h = 40
x = 5
y = 40
text = ""

widgetCounter = 0

numColumns = 1
fileNumber = 1
screen = phoebusgen.screen.Screen(bobFile + str(fileNumber))
screen.background_color(200, 200, 200)
for name, nodes in structure:
    # write box
    boxHeight = len(nodes) * 25 + 10 + maxLabelHeight
    if (y + boxHeight) > maxScreenHeight:
        y = 40
        numColumns += 1
        if (w + boxWidth + 5) > maxScreenWidth:
            screen.write_screen(bobFile + str(fileNumber) + ".bob")
            w += 10
            numColumns = 1
            fileNumber += 1
            text = ""
            w = boxWidth
            x = 5
            h = 40
            screen = phoebusgen.screen.Screen(bobFile + str(fileNumber))

        else:
            w += boxWidth + 5
            x += boxWidth + 5
    headingY = y + 5
    headingX = x + 5
    headingWidth = boxWidth - 10
    rect = phoebusgen.widget.Rectangle(
        f"Widget{widgetCounter}", x, y, boxWidth, boxHeight
    )
    rect.line_color(0, 0, 0)
    rect.background_color(175, 175, 175)
    screen.add_widget([rect])
    widgetCounter += 1
    y += 10 + headingHeight
    h = max(y, h)
    for node in nodes:
        nodeName = str(node.getAttribute("Name"))
        recordName = records[nodeName]
        ro = is_node_readonly(node)
        desc = ""
        for n in elements(node):
            if str(n.nodeName) in ["ToolTip", "Description"]:
                desc = getText(n)
        descs = ["%s: " % nodeName, "", "", "", "", ""]
        i = 0
        for word in desc.split():
            if len(descs[i]) + len(word) > 80:
                i += 1
                if i >= len(descs):
                    break
            descs[i] += word + " "
        for i in range(6):
            if descs[i]:
                globals()["desc%d" % i] = quoteString(descs[i])
            else:
                globals()["desc%d" % i] = "''"
        nx = x + 5
        # text += make_description()
        # nx += 10
        tlen = len(nodeName) * 10.0 / labelWidth
        if tlen <= 1.0:
            labelHeight = 20
        elif tlen <= 1.3:
            labelHeight = 16
        elif tlen <= 1.5:
            labelHeight = 14
        elif tlen <= 1.7:
            labelHeight = 12
        else:
            labelHeight = 10
        screen.add_widget(
            [
                phoebusgen.widget.Label(
                    f"Widget{widgetCounter}",
                    nodeName,
                    nx,
                    y,
                    labelWidth,
                    labelHeight,
                )
            ]
        )
        widgetCounter += 1
        nx += labelWidth + 5
        if ro:
            screen.add_widget(
                [
                    phoebusgen.widget.TextUpdate(
                        f"Widget{widgetCounter}",
                        f"$(P)$(R){recordName}",
                        nx,
                        y,
                        readonlyWidth,
                        readonlyHeight,
                    )
                ]
            )
            widgetCounter += 1
        elif node.nodeName in [
            "Integer",
            "IntReg",
            "Float",
            "Converter",
            "IntConverter",
            "IntSwissKnife",
            "SwissKnife",
            "StringReg",
            "String",
        ]:
            screen.add_widget(
                [
                    phoebusgen.widget.TextEntry(
                        f"Widget{widgetCounter}",
                        f"$(P)$(R){recordName}",
                        nx,
                        y,
                        textEntryWidth,
                        textEntryHeight,
                    )
                ]
            )
            widgetCounter += 1
            nx += textEntryWidth + 5
            screen.add_widget(
                [
                    phoebusgen.widget.TextUpdate(
                        f"Widget{widgetCounter}",
                        f"$(P)$(R){recordName}",
                        nx,
                        y,
                        readbackWidth,
                        readbackHeight,
                    )
                ]
            )
            widgetCounter += 1
        elif node.nodeName in ["Enumeration", "Boolean"]:
            screen.add_widget(
                [
                    phoebusgen.widget.ChoiceButton(
                        f"Widget{widgetCounter}",
                        f"$(P)$(R){recordName}",
                        nx,
                        y,
                        menuWidth,
                        menuHeight,
                    )
                ]
            )
            widgetCounter += 1
            nx += menuWidth + 5
            screen.add_widget(
                [
                    phoebusgen.widget.TextUpdate(
                        f"Widget{widgetCounter}",
                        f"$(P)$(R){recordName}",
                        nx,
                        y,
                        readbackWidth,
                        readbackHeight,
                    )
                ]
            )
            widgetCounter += 1
        elif node.nodeName in ["Command"]:
            screen.add_widget(
                [
                    phoebusgen.widget.ActionButton(
                        f"Widget{widgetCounter}",
                        nodeName,
                        f"$(P)$(R){recordName}",
                        nx,
                        y,
                        messageButtonWidth,
                        messageButtonHeight,
                    )
                ]
            )
            widgetCounter += 1
        else:
            print("Don't know what to do with", node.nodeName)
        y += 25
    y += 5
    h = max(y, h)

w += 10
screen.write_screen(bobFile + str(fileNumber) + ".bob")
