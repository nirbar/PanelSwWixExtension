# README

PanelSwWixExtension is a [WiX](http://wixtoolset.org/) extension that contains various addition to the build-in WiX toolset:

## WiX Elements

- Immediate Actions:
  - *SqlSearch*: Execute a SQL query and place result in a property.
  - *ReadIniValues*: Reads .INI file values.
  - *XmlSearch*: Read XML values.
  - *RegularExpression*: Execute a Regular Expression to replace or find matches in property values.
  - *MsiSqlQuery*: Execute a MSI-SQL query on the MSI database.
- Deferred Actions:
  - *Dism*: Enable Windows Feature using DISM API. Features will be installed when the parent component is being installed or repaired.
  - *ZipFile*: Creates a ZIP archive from selected files.
  - *Unzip*: Extract a ZIP archive to selected folder.
  - *CustomUninstallKey*: Overwrite registry values in the product's [Uninstall](http://msdn.microsoft.com/en-us/library/aa372105%28v=vs.85%29.aspx) registry key
  - *RemoveRegistryValue*: Removes registry values. Complements the standard [RemoveRegistryValue](http://wixtoolset.org/documentation/manual/v3/xsd/wix/removeregistryvalue.html) WiX element that will only remove registry values during installation.
  - *ExecOn*: Execute a custom command on component action.
  - *TaskScheduler*: Add a task to Windows Task Scheduler. Task definition XML should be in inner text.
  - *DeletePath*: Delete folder or file specified by a path.
  - *FileRegex*: Execute a Regular Expression to perform find & replace operations within files.
  - *ShellExecute*: Call ShellExecuteEx with parameters.
  - *Telemetry*: Send telemetry data to a given URL.
  - *InstallUtil*: Install a .NET assembly service.
  - *BackupAndRestore*: Backup a file before install or upgarde and restore it after.

## Custom Actions

- *TerminateSuccessfully_Immediate*: Terminates the installation with a successful indication. Executed in-script.
- *TerminateSuccessfully_Deferred*: Terminates the installation with a successful indication. Deferred execution.
- *SplitPath*: Split a full path supplied in 'FULL_PATH_TO_SPLIT' property and places the parts in properties SPLIT_DRIVE, SPLIT_FOLDER, SPLIT_FILE_NAME, SPLIT_FILE_EXTENSION.
- *PathExists*: Tests whether the path supplied in 'FULL_PATH_TO_TEST' property exists. Sets 'PATH_EXISTS' property to 1 if it exists or clears it if it doesn't.
- *SplitString*: Splits a string. Property name specified in 'PROPERTY_TO_SPLIT' and split token specified in 'STRING_SPLIT_TOKEN'. Results are stored in properties following the name supplied in 'PROPERTY_TO_SPLIT'.
  For example, say PROPERTY_TO_SPLIT="MY_PROP", MY_PROP="1,2,3" and STRING_SPLIT_TOKEN=",". Scheduling SplitString custom action will yield properties MY_PROP_0="1", MY_PROP_1="2" and MY_PROP_2="3".
- *TrimString*: Trim whitespace characters in property with name specified in 'PROPERTY_TO_TRIM'.

## Properties

The following properties hold localized built-in account names. To use them, add a [PropertyRef](http://wixtoolset.org/documentation/manual/v3/xsd/wix/propertyref.html) element.

- DOMAIN_ADMINISTRATORS
- DOMAIN_GUESTS
- DOMAIN_USERS
- ENTERPRISE_DOMAIN_CONTROLLERS
- DOMAIN_DOMAIN_CONTROLLERS
- DOMAIN_COMPUTERS
- BUILTIN_ADMINISTRATORS
- BUILTIN_GUESTS
- BUILTIN_USERS
- LOCAL_ADMIN
- LOCAL_GUEST
- ACCOUNT_OPERATORS
- BACKUP_OPERATORS
- PRINTER_OPERATORS
- SERVER_OPERATORS
- AUTHENTICATED_USERS
- PERSONAL_SELF
- CREATOR_OWNER
- CREATOR_GROUP
- LOCAL_SYSTEM
- POWER_USERS
- EVERYONE
- REPLICATOR
- INTERACTIVE
- NETWORK
- SERVICE
- RESTRICTED_CODE
- WRITE_RESTRICTED_CODE
- ANONYMOUS
- SCHEMA_ADMINISTRATORS
- CERT_SERV_ADMINISTRATORS
- RAS_SERVERS
- ENTERPRISE_ADMINS
- GROUP_POLICY_ADMINS
- ALIAS_PREW2KCOMPACC
- LOCAL_SERVICE
- NETWORK_SERVICE
- REMOTE_DESKTOP
- NETWORK_CONFIGURATION_OPS
- PERFMON_USERS
- PERFLOG_USERS
- IIS_USERS
- CRYPTO_OPERATORS
- OWNER_RIGHTS
- EVENT_LOG_READERS
- ENTERPRISE_RO_DCs
- CERTSVC_DCOM_ACCESS

## Building

PanelSwWixExtension require the following prerequisites to build:
- CMake: CMake path can be specified in 'CMakeDir' property in 'TidyBuild.custom.props'
- ADK installed. Specifically, Dism API should be installed. Set DismApi folder path in 'DismApiDir' property in 'TidyBuild.custom.props'
- Update git submodule protobuf

To build the extension download the code, open the solution PanelSwWixExtension.sln in Visual Studio 2017 and build it.

Unit-test WiX are available in the solution folder 'UnitTests'.