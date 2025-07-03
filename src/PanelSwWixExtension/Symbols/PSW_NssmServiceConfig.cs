using System;
using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_NssmServiceConfig : BaseSymbol
    {
        public enum AppExitType
        {
            Restart,
            Ignore,
            Exit,
        }

        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_NssmServiceConfig), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_NssmServiceConfig));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(ServiceName), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Application), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(AppDirectory), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(AppParameters), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(AppExit), ColumnType.String, 2, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.None),
                };
            }
        }

        public PSW_NssmServiceConfig() : base(SymbolDefinition)
        { }

        public PSW_NssmServiceConfig(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "svc")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string ServiceName
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Application
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string AppDirectory
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public string AppParameters
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }

        public AppExitType AppExit
        {
            get => (AppExitType)Enum.Parse(typeof(AppExitType),Fields[5].AsString());
            set => this.Set(5, value.ToString());
        }
    }
}
