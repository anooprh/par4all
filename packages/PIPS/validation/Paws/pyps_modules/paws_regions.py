
from pyps import workspace, module

def invoke_function(fu, ws):
        return fu._get_code(activate = module.print_code_regions)

if __name__=="__main__":
	workspace.delete('paws_regions')
	with workspace('paws_regions.c',name='paws_regions',deleteOnClose=True) as ws:
        	for fu in ws.fun:
                	print invoke_function(fu, ws)
