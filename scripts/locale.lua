--[[##################################################################################

	������ ������������� ������� ������ ��� ���� ��������� ����� "numeric"
	���� ������ ������ ������ � ����� �����, � ��� �� ����� ��������� ��� �������
	
	������ ����� ������ ��������� ��������� ������ ������������� ������.
	���� � ���, ��� � ���� ���������� ���� ���� ������������ ������� �����������
	������������� ������, � � ����� ������������ ������ ���� ������ :)

###################################################################################]]

path,selfname = debug.getinfo(1).source:match("^@?(.+[/\\])(.-)$")
assert(path, "Unable to detect operating system")


-- ��� ������� ���������� ������ � ����� ����
function OnStartup()
	local save
	while ScriptMan.MoveUp(selfname) do save = true end
	if save then SetMan.Save() end

	if path:sub(1,1) == "/" then	-- Linux
		for i, v in ipairs({"numeric","collate", "ctype"}) do
			local mustbe = i == 1 and "C" or SetMan.GetString(38)
			local current = os.setlocale(nil, v)
			if mustbe ~= current then
				local comma = ", "
				if not err then
					err = "������� ����������� ������ ��� ��������� ���������: "
					comma = ""
				end
				err = err..comma..v.." (����������� "..current..", ������ ���� "..mustbe..")"
			end
		end
	else	-- Windows
		local locale = SetMan.GetString(37)
		if not os.setlocale(locale) then
			err = "���������� ���������� ������ "..locale
		end
		os.setlocale("C","numeric")
	end
	if not err and ("����") ~= ("����"):lower() then
		err = "����������� �������� �������������� ���������"
	end
	if not err then
		Core.RegBot("�������","","",false)
		if Core.RegBot("�������","","",false) then
			Core.UnregBot("�������")
			err = "����������� �������� ������� strcasecmp"
		end
		Core.UnregBot("�������")
	end
	if err then
		TmrMan.AddTimer(1000*60*3, "DelayedReport")
		error(err)
	end
end

function DelayedReport(tmr)
	Core.SendToOpChat("������: "..err..". �����, ������� ��� ����������!")
	TmrMan.RemoveTimer(tmr)
end
