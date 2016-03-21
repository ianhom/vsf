If Wscript.Arguments.Count <> 1 Then
	MsgBox "Format: wscript fixsize.vbs FILE"
	Wscript.Quit
End If

Dim Stream, FileName, FileSize, Buffer, Str
FileName = Wscript.Arguments(0)
Set Stream = CreateObject("Adodb.Stream")

With Stream
	.Type = 1: .Mode = 3: .Open: .LoadFromFile FileName: Buffer = .Read
End With

FileSize = Stream.Size
Buffer = Str2BinArr(Long2Str(FileSize))

Stream.Position = 8
Stream.Write Buffer
Stream.SaveToFile FileName, 2
Stream.Close
Set Stream = Nothing
Wscript.Quit


Function Long2Str(Value)
	Dim i, Str, TmpByte

	For i = 0 To 3
		TmpByte = (Value / (2 ^ (i * 8))) And 255
		if TmpByte < 16 Then Str = Str + "0"
		Str = Str & Hex(CByte(TmpByte))
	Next
	Long2Str = Str
End Function

Function Str2BinArr(Str)
	Dim XML, Element

	Set XML = CreateObject("Microsoft.XMLDOM")
	Set Element = XML.CreateElement("None")
	Element.DataType = "bin.hex"
	Element.NodeTypedValue = Str
	Str2BinArr = Element.NodeTypedValue
	Set Element = Nothing
	Set XML = Nothing
End Function
