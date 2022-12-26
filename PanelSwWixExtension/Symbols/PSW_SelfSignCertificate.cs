using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_SelfSignCertificate : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_SelfSignCertificate), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_SelfSignCertificate));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column)
                    , new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1)
                    , new ColumnDefinition(nameof(X500), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(SubjectAltNames), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Expiry), ColumnType.Number, 4, false, false, ColumnCategory.Integer, 0, ushort.MaxValue)
                    , new ColumnDefinition(nameof(Password), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(DeleteOnCommit), ColumnType.Number, 2, false, false, ColumnCategory.Integer, 0, 1)
                };
            }
        }

        public PSW_SelfSignCertificate() : base(SymbolDefinition)
        { }

        public PSW_SelfSignCertificate(SourceLineNumber lineNumber, string id) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, id))
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string X500
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string SubjectAltNames
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public ushort Expiry
        {
            get => (ushort)Fields[3].AsNumber();
            set => Fields[3].Set(value);
        }

        public string Password
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }

        public int DeleteOnCommit
        {
            get => Fields[5].AsNumber();
            set => Fields[5].Set(value);
        }
    }
}
