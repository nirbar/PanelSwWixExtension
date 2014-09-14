# README #

PanelSwWixExtension is a [WiX](http://wixtoolset.org/) extension that contains various addition to the build-in WiX toolset:

WiX Elements:

1. *ReadIniValues*: Reads .INI file values.

1. *CustomUninstallKey*: Overwrite registry values in the product's [Uninstall](http://msdn.microsoft.com/en-us/library/aa372105%28v=vs.85%29.aspx) registry key

1. *RemoveRegistryValue*: Removes registry values. Complements the standard [RemoveRegistryValue](http://wixtoolset.org/documentation/manual/v3/xsd/wix/removeregistryvalue.html) WiX element that will only remove registry values during uninstall.


Custom Actions:

1. *TerminateSuccessfully_Immediate*: Terminates the installation with a successful indication. Executed in-script.

1. *TerminateSuccessfully_Deferred*: Terminates the installation with a successful indication. Deferred execution.

To build the extension download the code, open the solution PanelSwWixExtension.sln and build it in Visual Studio.

Unit-test WiX are available in the solution folder 'UnitTests'.