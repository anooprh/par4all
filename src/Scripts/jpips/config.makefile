#
# $Id$
#
# JPips is a java interface to tpips. It is based on swing.
#

jdk_dir	=	/usr/local/java1.2
jdk_bin	=	$(jdk_dir)/bin
jdk_lib	=	$(jdk_dir)/lib
pkg_dir	= 	classes
doc_dir	=	doc

#
# global definitions
#

J_FILES	=	\
		ObservableStream.java \
		StreamObserver.java \
		Console.java \
		Pawt.java \
		JPipsComponent.java \
		Displayer.java \
		TextDisplayer.java \
		GraphDisplayer.java \
		EmacsDisplayer.java \
		Resetable.java \
		Activatable.java \
		Stateable.java \
		Requestable.java \
		Listener.java \
		Watcher.java \
		TPips.java \
		Option.java \
		Parser.java \
		OptionParser.java \
		DirectoryManager.java \
		ModuleManager.java \
		WorkspaceManager.java \
		JPips.java 

MENUS	=	jpips_menus.test \
		jpips_menus.basic \
		jpips_menus.corinne \
		jpips_menus.developer

GIF	=	DownArrow.gif

SCRIPTS	=	jpips

SOURCES	=	$(J_FILES) $(SCRIPTS) $(MENUS) $(GIF)

JZIP	=	jpips.zip
JJAR	=	jpips.jar

INSTALL_SHR =	$(MENUS) $(GIF) $(JJAR) $(SCRIPTS)

#
# default rule
all: $(pkg_dir) $(JJAR)

#
# compilation and related stuff

package	=	JPips
d	=	$(pkg_dir)/$(package)

$(pkg_dir):; mkdir $(pkg_dir)

OJ_FILES=	$(addprefix $(d)/, $(J_FILES:.java=.class))

# local stuff

#swing	=	/usr/local/swing/swingall.jar
#jdk	=	$(jdk_lib)/classes.zip

JENV	=	CLASSPATH=.:classes
#:$(swing):$(jdk)
JAVAC	=	$(jdk_bin)/javac
JCFLAGS	=	-g -deprecation -d $(pkg_dir)

JAVA	=	$(jdk_bin)/java
JFLAGS	=	-Djava.compiler=sunwjit

JAR	=	jar
JARFLAGS=	cvf

ZIP	=	zip
ZIPFLAGS=	-9 -v

JDOC	=	$(jdk_bin)/javadoc
JDFLAGS	=	-author -d $(doc_dir)

# default rule for compiling java files.
$(d)/%.class: %.java
	$(JENV) $(JAVAC) $(JCFLAGS) $< \
		&& chmod ug+rw $(pkg_dir)/$(package)/$**

.doc:
	$(JDOC) $(JDFLAGS) $(J_FILES) && touch $@

# jpips.jar: $(OJ_FILES)
# 	$(JENV) $(JAR) $(JARFLAGS) $@ $(pkg_dir)/$(package)/*.class

$(JZIP): $(OJ_FILES)
	pwd=`pwd` ; cd $(pkg_dir); \
		$(ZIP) $(ZIPFLAGS) $$pwd/$@ $(package)/*.class

# jar files are a little bigger than simple zip files.
# $(JAR) $(JARFLAGS) $@ -C $(pkg_dir) $(pkg_dir)/$(package)/*.class

$(JJAR): $(OJ_FILES)
	pwd=`pwd` ; cd $(pkg_dir); \
		$(JAR) $(JARFLAGS) $$pwd/$@ $(package)/*.class


zrun: $(JZIP)
	CLASSPATH=$(JZIP) $(JAVA) $(JFLAGS) $(package).JPips

jrun: $(JJAR)
	CLASSPATH=$(JJAR) $(JAVA) $(JFLAGS) $(package).JPips

clean:local-clean
local-clean:; $(RM) $(OJ_FILES) $(JJAR) $(JZIP); $(RM) -rf $(pkg_dir)

#
# java module dependencies (set manually)
#

$(d)/Activatable.class:
$(d)/Stateable.class:
$(d)/Resetable.class:
$(d)/Requestable.class:
$(d)/StreamObserver.class:

$(d)/ObservableStream.class: \
	$(d)/StreamObserver.class

$(d)/Console.class: \
	$(d)/ObservableStream.class \
	$(d)/StreamObserver.class

$(d)/Pawt.class: \
	$(d)/Activatable.class

$(d)/JPipsComponent.class: \
	$(d)/Activatable.class \
	$(d)/Pawt.class \
	$(d)/Stateable.class

$(d)/Displayer.class: \
	$(d)/JPipsComponent.class

$(d)/TextDisplayer.class: \
	$(d)/Pawt.class \
	$(d)/Displayer.class 

$(d)/GraphDisplayer.class: \
	$(d)/Pawt.class \
	$(d)/Displayer.class 

$(d)/EmacsDisplayer.class: \
	$(d)/Pawt.class \
	$(d)/Displayer.class 

$(d)/Listener.class: \
	$(d)/Requestable.class

$(d)/Watcher.class: \
	$(d)/Requestable.class

$(d)/TPips.class: \
	$(d)/Listener.class \
	$(d)/Watcher.class \
	$(d)/Stateable.class \
	$(d)/Requestable.class

$(d)/Option.class: \
	$(d)/TPips.class \
	$(d)/Stateable.class

$(d)/Parser.class: \
	$(d)/Option.class

$(d)/OptionParser.class: \
	$(d)/Parser.class \
	$(d)/Option.class

$(d)/DirectoryManager.class: \
	$(d)/TPips.class \
	$(d)/Pawt.class \
	$(d)/JPipsComponent.class

$(d)/ModuleManager.class: \
	$(d)/TPips.class \
	$(d)/Pawt.class \
	$(d)/JPipsComponent.class

$(d)/WorkspaceManager.class: \
	$(d)/TPips.class \
	$(d)/Pawt.class \
	$(d)/JPipsComponent.class

$(d)/JPips.class: \
	$(d)/DirectoryManager.class \
	$(d)/ModuleManager.class \
	$(d)/WorkspaceManager.class \
	$(d)/TPips.class \
	$(d)/TextDisplayer.class \
	$(d)/GraphDisplayer.class \
	$(d)/EmacsDisplayer.class \
	$(d)/OptionParser.class
