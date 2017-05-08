using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

using PanelSw.Wix.Extensions;
using Microsoft.Tools.WindowsInstallerXml;

[assembly: AssemblyTitle("PanelSw WiX Extension")]
[assembly: AssemblyDefaultWixExtension(typeof(PanelSwWixExtension))]

