# for module compiling
Import('RTK_SDK_ROOT')
Import('RTK_IC_TYPE')
import os
from building import *
import menu_config

# get current directory
cwd  = GetCurrentDir()
objs = []
list = os.listdir(cwd)
parent_dir = os.path.dirname(cwd)

src = Split("""
""")

include_path = []
libs = ['']

if GetDepend(['CONFIG_REALTEK_LCDC_DBIB']):
    src += ['hardware/lcdc/src/device/rtl_common/rtl_lcdc_dbib.c']
if GetDepend(['CONFIG_REALTEK_LCDC_DBIC']):
    src += ['hardware/lcdc/src/device/rtl_common/rtl_lcdc_dbic.c']
if GetDepend(['CONFIG_REALTEK_LCDC_DSI']):
    src += ['hardware/mipi/src/device/rtl_common/rtl_lcdc_dsi.c']
if GetDepend(['CONFIG_REALTEK_LCDC_EDPI']) :
    src += ['hardware/lcdc/src/device/rtl_common/rtl_lcdc_edpi.c']
if GetDepend(['CONFIG_REALTEK_LCDC']):
    src += ['hardware/lcdc/src/device/rtl_common/rtl_lcdc.c']
if GetDepend(['CONFIG_REALTEK_PPE']):
    src += ['hardware/ppe/src/device/' + RTK_IC_TYPE + '/rtl_ppe.c']
if GetDepend(['CONFIG_REALTEK_RAMLESS_QSPI']):
    src += ['hardware/lcdc/src/device/rtl_common/rtl_ramless_qspi.c']

if  GetDepend(['CONFIG_REALTEK_IDU']) :
    src += ['hardware/idu/src/device/rtl_common/rtl_idu.c']
    src += ['hardware/idu/src/device/' + RTK_IC_TYPE + '/rtl_idu_int.c']

if GetDepend(['CONFIG_REALTEK_SEGCOM']):
    src += ['hardware/segcom/device/src/rtl_common/rtl876x_segcom.c']


if GetDepend(['CONFIG_REALTEK_BUILD_PPE']):
    src += ['bindings/honeygui/hardware_engine/PPE/' + RTK_IC_TYPE + '/acc_ppe.c']

include_path += [cwd,
        cwd + '/hardware/lcdc/inc',
        cwd + '/hardware/lcdc/src/device/' + RTK_IC_TYPE,
        cwd + '/hardware/idu/inc',
        cwd + '/hardware/idu/src/device/' + RTK_IC_TYPE,
        cwd + '/hardware/idu/inc',
        cwd + '/hardware/mipi/inc',
        cwd + '/hardware/ppe/inc/' + RTK_IC_TYPE,
        cwd + '/hardware/ppe/src/device/' + RTK_IC_TYPE,
        cwd + '/hardware/segcom/inc']


group = DefineGroup('peripheral', src, depend = [''], CPPPATH = include_path,LIBS = [''], LIBPATH = [''])

for d in list:
    path = os.path.join(cwd, d)
    if os.path.isfile(os.path.join(path, 'SConscript')):
        group = group + SConscript(os.path.join(d, 'SConscript'))

Return('group')