--[[##################################################################################

	Скрипт устанавливает русскую локаль для всех категорий кроме "numeric"
	Этот скрипт должен стоять в самом верху, и его не нужно отключать или удалять
	
	Помимо этого скрипт выполняет несколько весьма занимательных тестов.
	Дело в том, что в один прекрасный день меня окончательно заебали неправильно
	установленные локали, и я решил окончательно решить этот вопрос :)

###################################################################################]]

path,selfname = debug.getinfo(1).source:match("^@?(.+[/\\])(.-)$")
assert(path, "Unable to detect operating system")


-- При запуске перемещаем скрипт в самый верх
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
					err = "Неверно установлена локаль для следующих категорий: "
					comma = ""
				end
				err = err..comma..v.." (установлена "..current..", должна быть "..mustbe..")"
			end
		end
	else	-- Windows
		local locale = SetMan.GetString(37)
		if not os.setlocale(locale) then
			err = "Невозможно установить локаль "..locale
		end
		os.setlocale("C","numeric")
	end
	if not err and ("тест") ~= ("ТЕСТ"):lower() then
		err = "Некорректно работает преобразование регистров"
	end
	if not err then
		Core.RegBot("тестбот","","",false)
		if Core.RegBot("ТЕСТБОТ","","",false) then
			Core.UnregBot("ТЕСТБОТ")
			err = "Некорректно работает функция strcasecmp"
		end
		Core.UnregBot("тестбот")
	end
	if err then
		TmrMan.AddTimer(1000*60*3, "DelayedReport")
		error(err)
	end
end

function DelayedReport(tmr)
	Core.SendToOpChat("ОШИБКА: "..err..". Админ, исправь это немедленно!")
	TmrMan.RemoveTimer(tmr)
end
