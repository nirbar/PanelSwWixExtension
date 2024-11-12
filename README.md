[![PanelSwWixExtension](https://github.com/nirbar/PanelSwWixExtension/actions/workflows/github-actions-build.yml/badge.svg?branch=master)](https://github.com/nirbar/PanelSwWixExtension/actions/workflows/github-actions-build.yml)
WiX v3 [![NuGet version (PanelSwWixExtension)](https://img.shields.io/nuget/v/PanelSwWixExtension.svg?style=flat-square)](https://www.nuget.org/packages/PanelSwWixExtension/)
WiX v4 [![NuGet version (PanelSwWixExtension4)](https://img.shields.io/nuget/v/PanelSwWixExtension4.svg?style=flat-square)](https://www.nuget.org/packages/PanelSwWixExtension4/)

PanelSwWixExtension from the creator of [JetBA](https://github.com/nirbar/JetBA-Binaries) is the most comprehensive open source [WiX](http://wixtoolset.org/) extension. 
I would like to thank JetBrains for their [support](https://www.jetbrains.com/community/opensource/) of this project

[Contact me](https://www.panel-sw.com/) should you require any additional features, custom WiX fixes and enhancements, or for license to the commercial JetBA WiX WPF UI framework

## PanelSwWixExtension WiX Elements

- Immediate Actions:
  - *DuplicateFolder*: Duplicate an entire folder hierarchy. Processed at build time by adding entries to Directory, CreateFolder and DuplicateFile tables.
  - *IsWindowsVersionOrGreater*: Test whether Kernel32.dll's version is at least the given version. If yes, set the property to 1.
  - *Md5Hash*: Computes MD5 hash.
  - *WmiSearch*: Execute a WMI query.
  - *PathSearch*: Search for a file on PATH environment variable folder list.
  - *VersionCompare*: Compare two versions, set result to property in range -4 to +4, denoting the version part that was different.
  - *ForceVersion*: Force a specified version for a file. Version overwrite is done on runtime in the MSI database.
  - *AccountSidSearch*: Lookup an account's SID by account name.
  - *JsonJpathSearch*: Read JSON values
  - *DiskSpace*: Calculate available disk space for a target directory
  - *CertificateHashSearch*: Find a certificate hash in local system MY store.
  - *Evaluate*: Evaluate mathematical expressions and store result in property.
  - *SetPropertyFromPipe*: Allows setting property from a pipe.
  - *SqlSearch*: Execute a SQL query and place result in a property.
  - *ReadIniValues*: Reads .INI file values.
  - *XmlSearch*: Read XML values.
  - *RegularExpression*: Execute a Regular Expression to replace or find matches in property values and in file contents.
  - *MsiSqlQuery*: Execute a MSI-SQL query on the MSI database.
  - *CreateSelfSignCertificate*: Create a self-sign certificate that can then be installed by WixIisExtension.
  - *ToLowerCase*: Convert property value to lowercase.
  - *CustomPatchRef* Support referencing custom table rows in a patch
  - *Payload* Support extracting files from Binary table temporarilty during MSI execution
  - *PromptFileDowngrades* Log each file that will be downgraded during the (re)install, and prompt the total downgrade count if larger than 0.
- Deferred Actions:
  - *RemoveFolderEx*: Remove a folder recursively. Differs from WixUtilExtension's RemoveFolderEx in proper handling of reparse points (symbolic links, mount volumes, etc.)
  - *ExecuteCommand*: Launch a deferred command
  - *RestartLocalResources*: Register processes with the Restart Manager if they reside in the specified folder
  - *XslTransform*: Apply a XSL transform on an installed XML file
  - *WebsiteConfig*: Start or stop a website
  - *ApplicationPoolConfig*: Start or stop an IIS application pool
  - *JsonJPath*: Set values in JSON-formatted file.
  - *Dism*: Enable or disable Windows Features using DISM API. Features will be modified when the parent component is being installed or repaired.
  - *ZipFile*: Creates a ZIP archive from selected files.
  - *Unzip*: Extract a ZIP archive to selected folder.
  - *CustomUninstallKey*: Overwrite registry values in the product's [Uninstall](http://msdn.microsoft.com/en-us/library/aa372105%28v=vs.85%29.aspx) registry key
  - *RemoveRegistryValue*: Removes registry values. Complements the standard [RemoveRegistryValue](http://wixtoolset.org/documentation/manual/v3/xsd/wix/removeregistryvalue.html) WiX element that will only remove registry values during installation.
  - *ExecOn*: Execute a custom command on component action.
  - *TaskScheduler*: Add a task to Windows Task Scheduler.
  - *DeletePath*: Delete folder or file specified by a path.
  - *FileRegex*: Execute a Regular Expression to perform find & replace operations within files.
  - *ShellExecute*: Call ShellExecuteEx with parameters.
  - *Telemetry*: Send telemetry data to a given URL.
  - *InstallUtil*: Install a .NET assembly service.
  - *TopShelf*: Install a [TopShelf](http://topshelf-project.com/) based service.
  - *BackupAndRestore*: Backup a file before install or upgarde and restore it after.
  - *SqlScript*: Execute SQL scripts, optionally with text replacements.
  - *SplitFile*: Splits a file to parts during build, and reconstruct it during install. Used to workaround MSI limitation of 2GB file size.
- Bundle Actions:
  - *ExePackage/UninstallCommand*: Execute a custom command to uninstall an ExePackage. Used for packages which the uninstall is launched via a different executable than the install.

## Preprocessor
- *$(psw.VarNullOrEmpty(VAR_NAME))*: Return 1 if the variable is not defined, or if it defined as empty. Return 0 otherwise
- *$(psw.AutoGuid(x,y,z,...))*: Return a pseudo-random UUID per arguments
- *$(psw.FileExists(path))*: Return 1 if the file exists, 0 otherwise
- *$(psw.DirExists(path))*: Return 1 if the folder exists, 0 otherwise
- *$(psw.DirEmpty(path))*: Return 1 if the folder doesn't exist or if it has no files (including in subfolders). Return 0 otherwise

## Custom Actions

- *TerminateSuccessfully_Immediate*: Terminates the installation with a successful indication. Executed in-script.
- *TerminateSuccessfully_Deferred*: Terminates the installation with a successful indication. Deferred execution.
- *SplitPath*: Split a full path supplied in 'FULL_PATH_TO_SPLIT' property and places the parts in properties SPLIT_DRIVE, SPLIT_FOLDER, SPLIT_FILE_NAME, SPLIT_FILE_EXTENSION.
- *PathExists*: Tests whether the path supplied in 'FULL_PATH_TO_TEST' property exists. Sets 'PATH_EXISTS' property to 1 if it exists or clears it if it doesn't.
- *SplitString*: Splits a string. Property name specified in 'PROPERTY_TO_SPLIT' and split token specified in 'STRING_SPLIT_TOKEN'. Results are stored in properties following the name supplied in 'PROPERTY_TO_SPLIT'.
  For example, say PROPERTY_TO_SPLIT="MY_PROP", MY_PROP="1,2,3" and STRING_SPLIT_TOKEN=",". Scheduling SplitString custom action will yield properties MY_PROP_0="1", MY_PROP_1="2" and MY_PROP_2="3".
- *TrimString*: Trim whitespace characters in property with name specified in 'PROPERTY_TO_TRIM'.

## Error codes

PanelSwWixExtension uses error codes in Error table:

- 27000: TopShelf error template for prompting user on errors.
- 27001: ExecOn error template for prompting user on errors.
- 27002: ServiceConfig error template for prompting user on errors.
- 27003: Dism error template for prompting user on failures to add a Windows feature package.
- 27004: Dism error template for prompting user on failures to enable a Windows feature.
- 27005: SqlScript error template for prompting user on errors.
- 27006: ExecOn error template for prompting user on console output parsing.
- 27007: WebsiteConfig error template for prompting user on errors.
- 27008: SqlSearch error template for prompting user on errors.
- 27009: JsonJpath error template for prompting user on errors.
- 27010: Dism error template for prompting user on failures to remove a Windows feature package.
- 27011: ExecOn message template for prompting user on success with promptAlways.
- 27012: Prompt on file downgrades
- 27013: Prompt on failure to add a file to a ZIP archive
- 27014: Prompt on failure to create a ZIP archive
- 27015: Prompt on failure to unzip a ZIP archive
- 27016: Prompt on failure to unzip a file from ZIP archive
- 27017: ApplicationPoolConfig error template for prompting user on errors.

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

The following properties hold localized CSIDL folders. To use them, add a [PropertyRef](http://wixtoolset.org/documentation/manual/v3/xsd/wix/propertyref.html) element.

- UserDesktopFolder: CSIDL_DESKTOPDIRECTORY

The following [properties](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocessorfeaturepresent) denote available processor features. 1 means that the feature is available, 0 means that it is not available.

- PF_ARM_64BIT_LOADSTORE_ATOMIC
- PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE
- PF_ARM_EXTERNAL_CACHE_AVAILABLE
- PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE
- PF_ARM_VFP_32_REGISTERS_AVAILABLE
- PF_3DNOW_INSTRUCTIONS_AVAILABLE
- PF_CHANNELS_ENABLED
- PF_COMPARE_EXCHANGE_DOUBLE
- PF_COMPARE_EXCHANGE128
- PF_COMPARE64_EXCHANGE128
- PF_FASTFAIL_AVAILABLE
- PF_FLOATING_POINT_EMULATED
- PF_FLOATING_POINT_PRECISION_ERRATA
- PF_MMX_INSTRUCTIONS_AVAILABLE
- PF_NX_ENABLED
- PF_PAE_ENABLED
- PF_RDTSC_INSTRUCTION_AVAILABLE
- PF_RDWRFSGSBASE_AVAILABLE
- PF_SECOND_LEVEL_ADDRESS_TRANSLATION
- PF_SSE3_INSTRUCTIONS_AVAILABLE
- PF_SSSE3_INSTRUCTIONS_AVAILABLE
- PF_SSE4_1_INSTRUCTIONS_AVAILABLE
- PF_SSE4_2_INSTRUCTIONS_AVAILABLE
- PF_AVX_INSTRUCTIONS_AVAILABLE
- PF_AVX2_INSTRUCTIONS_AVAILABLE
- PF_AVX512F_INSTRUCTIONS_AVAILABLE
- PF_VIRT_FIRMWARE_ENABLED
- PF_XMMI_INSTRUCTIONS_AVAILABLE
- PF_XMMI64_INSTRUCTIONS_AVAILABLE
- PF_XSAVE_ENABLED
- PF_ARM_V8_INSTRUCTIONS_AVAILABLE
- PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE
- PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE
- PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE
- PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE
- PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE
- PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE

## Building

PanelSwWixExtension require the following prerequisites to build:
- CMake: CMake path can be specified in 'CMakeDir' property in 'TidyBuild.custom.props'
- ADK installed. Specifically, Dism API should be installed. Set DismApi folder path in 'DismApiDir' property in 'TidyBuild.custom.props'
- Update git submodules

To build the extension download the code, open the solution PanelSwWixExtension.sln in Visual Studio 2017 and build it.

Unit-test WiX are available in the solution folder 'UnitTests'.
After building a unit test project, you'll need to shutdown Visual Studio before you can build PanelSwWixExtension project again. 
This is due to the unfortunate habit of Visual Studio to hold the extension file in use.
You may find it convenient to build unit test projects from a command prompt to workaround this limitation
~~~~~~~~~~~~
MSBuild UnitTests\FileOperationsUT\FileOperationsUT.wixproj /p:Configuration=Release /p:Platform=x86 /t:Rebuild "/p:SolutionDir=%CD%\\"
~~~~~~~~~~~~
