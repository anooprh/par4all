# coding=iso-8859-15
import pypips
import os
import tempfile
import shutil
from string import split, upper, join

pypips.atinit()

class loop:
	"""a loop represent a doloop of  module"""

	def __init__(self,module,label):
		"""[[internal]] bind a loop to its module"""
		self.module=module
		self.label=label
		self.ws=module.ws

### loop_methods /!\ do not touch this line /!\


class module:
	"""a module represents a function, it is the basic element of pyps
		you can select modules from the workspace and apply transformations to them"""

	def __init__(self,ws,name,source=""):
		"""[[internal]] bind a module to its workspace"""
		self.name=name
		self.source=source
		self.ws=ws

	def show(self,rc):
		"""returns the name of resource rc"""
		return split(pypips.show(upper(rc),self.name))[-1]

	def apply(self,phase):
		"""apply transformation phase"""
		pypips.apply(upper(phase),self.name)

	def display(self,rc="printed_file",With="PRINT_CODE"):
		"""display a given resource rc"""
		self.ws.activate(With)
		return pypips.display(upper(rc),self.name)

	def code(self):
		"""return module code as a string"""
		self.apply("print_code")
		rcfile=self.show("printed_file")
		return file(self.ws.dir()+rcfile).readlines()

	def loops(self, label=""):
		"""return desired loop if label given, an iterator over loops otherwise"""
		self.apply("print_loops")
		rcfile=self.show("loops_file")
		return map(lambda line:loop(self,line[0:-1]), file(self.ws.dir()+rcfile).readlines()) if not label else loop(label)

	def _update_props(self,passe,props):
		"""[[internal]] change a property dictionnary by appending the passe name to the property when needed """
		for name,val in props.iteritems():
			if upper(name) not in self.all_properties:
				del props[upper(name)]
				props[upper(passe+"_"+name)]=val
				#print "warning, changing ", name, "into", passe+"_"+name
		return props

### module_methods /!\ do not touch this line /!\

class modules:
	"""high level representation of a module set,
	its only purpose is to dispatch maethod calls on contained modules"""
	def __init__(self,modules):
		self.modules=modules
		self.ws= modules[0].ws if modules else None

	def display(self,rc="printed_file", With="PRINT_CODE"):
		"""display all modules"""
		map(lambda m:m.display(rc, With),self.modules)

	def loops(self):
		""" return a list of all program loops"""
		return reduce(lambda l1,l2:l1+l2.loops(), self.modules, [])

### modules_methods /!\ do not touch this line /!\

class workspace:
	"""top level element of the pyps hierarchy,
		it represents a set of source files and provides methods
		to manipulate them"""

	def __build_module_list(self):
		for m in self.info("modules"):
			self.modules[m]=module(self,m,self.sources[0])

	def __init__(self,sources2,name="",activates=[],verboseon=True):
		"""init a workspace from a list of sources"""
		if name == "":
			name=os.path.basename(tempfile.mkdtemp("","PYPS"))
		def helper(x,y):
			return x+y if isinstance(y,list) else x +[y]
		self.sources=reduce(helper,sources2,[])
		pypips.create(name, self.sources)
		if not verboseon:self.set_property(USER_LOG_P=False)
		self.modules = {}
		map(lambda x:pypips.activate(x),activates)
		self.__build_module_list()
		self.name=self.info("workspace")[0]
		self.cleared=False

	def __iter__(self):
		"""provide an iterator on workspace's module, so that you can write
			map(do_something,my_workspace)"""
		return self.modules.itervalues()


	def __getitem__(self,module_name):
		"""retreive a module of the module from its name"""
		self.__build_module_list()
		return self.modules[module_name]

	def __setitem__(self,i):
		"""change a module of the module from its name"""
		return self.modules[i]

	def info(self,topic):
		return split(pypips.info(topic))

	def dir(self):
		"""retreive workspace datadir"""
		return self.name+".database/"

	def _set_property(self,props):
		"""[internal] set properties based on the dictionnary props"""
		for prop,value in props.iteritems():
			if type(value) is bool:
				val=upper(str(value))
			elif type(value) is str:
				def stringify(s): return '"'+s+'"'
				val=stringify(value)
			else:
				val=str(value)
			pypips.set_property(upper(prop),val)

	def set_property(self,**props):
		"""set multpiple properties at once"""
		self._set_property(props)

	def save(self,indir="",with_prefix=""):
		"""save worksapce back into source aither in directory indir or with the prefix with_prefix"""
		pypips.apply("UNSPLIT","%ALL")
		saved=[]
		if indir:
			if not os.path.exists(indir):
				os.makedirs(indir)
			if not os.path.isdir(indir): raise ValueError("'" + indir + "' is not a directory") 
			for s in os.listdir(self.dir()+"Src"):
				cp=os.path.join(indir,s)
				shutil.copy(os.path.join(self.dir(),"Src",s),cp)
				saved+=[cp]
		else:
			for s in os.listdir(self.dir()+"Src"):
				cp=with_prefix+s
				shutil.copy(os.path.join(self.dir(),"Src",s),cp)
				saved+=[cp]
		return saved

	def compile(self,CC="gcc",CFLAGS="-O2 -g", LDFLAGS="", link=True, outdir=".", outfile="",extrafiles=[]):
		"""try to compile current workspace, some extrafiles can be given with extrafiles list"""
		if not os.path.isdir(outdir): raise ValueError("'" + indir + "' is not a directory") 
		otmpfiles=self.save(indir=outdir)+extrafiles
		command=[CC,CFLAGS]
		if link:
			if not outfile:
				outfile=self.name
			command+=otmpfiles
			command+=[LDFLAGS]
			command+=["-o", outfile]
		else:
			command+=["-c"]
			command+=otmpfiles
		#print "running", " ".join(command)
		if os.system(" ".join(command)):
			if not link: map(os.remove,otmpfiles)
		return outfile

	def activate(self,phase):
		"""activate a given phase"""
		pypips.activate(phase)

	def filter(self,matching=lambda x:True):
		"""create an object containing current listing of all modules,
		filterd by the filter argument"""
		self.__build_module_list()
		the_modules=[m for m in self.modules.values() if matching(m)]
		return modules(the_modules)

	all=property(filter)

	def close(self):
		"""force cleaning and deletion of the workspace"""
		self.cleared=True
		pypips.quit()
		pypips.delete_workspace(self.name)

	def __del__(self):
		if not self.cleared:self.quit()

