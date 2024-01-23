using WixToolset.Data;

namespace PanelSw.Wix.Extensions
{
    public static class PanelSwWixErrorMessages
    {
        public static Message ExecuteCommandSequence(SourceLineNumber sourceLineNumber, string executeCommandId, string otherActionId)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.ExecuteCommandSequence, MessageResources.ExecuteCommandSequence, executeCommandId, otherActionId);
        }
        public static Message PayloadExceedsSize(SourceLineNumber sourceLineNumber, string payloadId, long maxSize)
        {
            return new Message(sourceLineNumber, MessageLevel.Warning, (int)PswErrorId.PayloadExceedsSize, MessageResources.PayloadExceedsSize, payloadId, maxSize);
        }
        public static Message ContainerError(SourceLineNumber sourceLineNumber, string containerId, string error)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.ContainerError, MessageResources.ContainerError, containerId, error);
        }
        public static Message PswWixAttribute(SourceLineNumber sourceLineNumber, string attribute, string element)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.PswWixAttribute, MessageResources.PswWixAttribute, attribute, element);
        }
        public static Message MissingContainerTemplate(SourceLineNumber sourceLineNumber, string containerId)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.MissingContainerTemplate, MessageResources.MissingContainerTemplate, containerId);
        }
    }

    public enum PswErrorId : int
    {
        ExecuteCommandSequence = 9000,
        PayloadExceedsSize,
        ContainerError,
        PswWixAttribute,
        MissingContainerTemplate,
    }
}
