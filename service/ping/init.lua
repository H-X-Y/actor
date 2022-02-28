function OnInit(id)
    print("[lua] ping OnInit id:"..id)
    serviceId = id
end

function OnServiceMsg(source,buff)
    print("[lua] ping OnserviceMsg id:"..serviceId)
    if string.len(buff) > 7 then
        server.KillService(serviceId)
        return
    end
    server.Send(serviceId,source,buff.."i")
end

function OnExit()
    print("[lua] ping OnExit")
end