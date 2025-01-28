using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_RemoveFolderEx : BaseSymbol
    {
        public enum RemoveFolderExInstallMode
        {
            Install = 1,
            Uninstall = 2,
            Both = 3,
        }

        public enum RemoveFolderExLongPathHandling
        {
            Default = 0,
            Ignore = 1,
            Rename = 2,
            Prompt = 3,
        }

        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_RemoveFolderEx), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_RemoveFolderEx));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property), ColumnType.String, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(InstallMode), ColumnType.Number, 0, false, false, ColumnCategory.Integer, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(LongPathHandling), ColumnType.Number, 0, false, false, ColumnCategory.Integer, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_RemoveFolderEx() : base(SymbolDefinition)
        { }

        public PSW_RemoveFolderEx(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "rmf")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Property
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public RemoveFolderExInstallMode InstallMode
        {
            get => (RemoveFolderExInstallMode)Fields[2].AsNumber();
            set => this.Set(2, (int)value);
        }

        public RemoveFolderExLongPathHandling LongPathHandling
        {
            get => (RemoveFolderExLongPathHandling)Fields[3].AsNumber();
            set => this.Set(3, (int)value);
        }

        public string Condition
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }
    }
}
