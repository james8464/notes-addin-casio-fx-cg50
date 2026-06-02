  /* PYTHON */

  string replace(const string & s,char c1,char c2){
    string res;
    int l=s.size();
    res.reserve(l);
    const char * ch=s.c_str();
    for (int i=0;i<l;++i,++ch){
      res+= (*ch==c1? c2: *ch);
    }
    return res;
  }

  static string remove_comment(const string & s,const string &pattern,bool rep){
    string res(s);
    for (;;){
      int pos1=res.find(pattern);
      if (pos1<0 || pos1+3>=int(res.size()))
	break;
      int pos2=res.find(pattern,pos1+3);
      if (pos2<0 || pos2+3>=int(res.size()))
	break;
      if (rep){
	string tmp=res.substr(0,pos1);
	tmp +=  '"';
	tmp += replace(res.substr(pos1+3,pos2-pos1-3),'\n',' ');
	tmp += '"';
	tmp += res.substr(pos2+3,res.size()-pos2-3);
	res = tmp;
      }
      else
	res = res.substr(0,pos1)+res.substr(pos2+3,res.size()-pos2-3);
    }
    return res;
  }

  struct int_string {
    int decal;
    std::string endbloc;
    int_string():decal(0){}
    int_string(int i,string s):decal(i),endbloc(s){}
  };

  static bool instruction_at(const string & s,int pos,int shift){
    if (pos && isalphan(s[pos-1]))
      return false;
    if (pos+shift<int(s.size()) && isalphan(s[pos+shift]))
      return false;
    return true;
  }

  void convert_python(string & cur,GIAC_CONTEXT){
    bool indexshift=array_start(contextptr); //xcas_mode(contextptr)!=0 || abs_calc_mode(contextptr)==38;
    if (cur[0]=='_' && (cur.size()==1 || !isalpha(cur[1])))
      cur[0]='@'; // python shortcut for ans(-1)
    bool instring=cur.size() && cur[0]=='"';
    int openpar=0;
    for (int pos=1;pos<int(cur.size());++pos){
      char prevch=cur[pos-1],curch=cur[pos];
      if (curch=='"' && prevch!='\\')
	instring=!instring;
      if (instring)
	continue;
      if (curch=='(')
	++openpar;
      if (curch==')')
	--openpar;
      if (curch==',' && pos<int(cur.size()-1)){
	char nextch=cur[pos+1];
	if (nextch=='}' || nextch==']' || nextch==')'){
	  cur.erase(cur.begin()+pos);
	  continue;
	}
      }
      if (curch=='}' && prevch=='{'){
	cur=cur.substr(0,pos-1)+"table()"+cur.substr(pos+1,cur.size()-pos-1);
	continue;
      }
      if (curch==':' && (prevch=='[' || prevch==',')){
	cur.insert(cur.begin()+pos,indexshift?'1':'0');
	continue;
      }
      if (curch==':' && pos<int(cur.size())-1 && cur[pos+1]!='=' && cur[pos+1]!=';'){
	int posif=cur.find("if "),curpos,cursize=int(cur.size()),count=0;
	// check is : for slicing?
	for (curpos=pos+1;curpos<cursize;++curpos){
	  if (cur[curpos]=='[')
	    ++count;
	  if (cur[curpos]==']')
	    --count;
	}
	if (count==0 && posif>=0 && posif<pos){
	  cur[pos]=')';
	  cur.insert(cur.begin()+posif+3,'(');
	  continue;
	}
      }
      if ( (curch==']' && (prevch==':' || prevch==',')) ||
	   (curch==',' && prevch==':') ){
	cur[pos-1]='.';
	cur.insert(cur.begin()+pos,'.');
	++pos;
	if (indexshift)
	  cur.insert(cur.begin()+pos,'0');
	else {
	  cur.insert(cur.begin()+pos,'1');
	  cur.insert(cur.begin()+pos,'-');
	}
	continue;
      }
      if (curch==':' && prevch==':'){
	if (pos+1<int(cur.size()) && cur[pos+1]=='-'){
	  cur.insert(cur.begin()+pos,'-');
	  cur.insert(cur.begin()+pos+1,'1');
	}
	else
	  cur.insert(cur.begin()+pos,'0');
	continue;	
      }
      if (curch=='%'){
	cur.insert(cur.begin()+pos+1,'/');
	++pos;
	continue;
      }
      if (curch=='=' && openpar==0 && prevch!='>' && prevch!='<' && prevch!='!' && prevch!=':' && prevch!=';' && prevch!='=' && prevch!='+' && prevch!='-' && prevch!='*' && prevch!='/' && prevch!='%' && (pos==int(cur.size())-1 || (cur[pos+1]!='=' && cur[pos+1]!='<'))){
	cur.insert(cur.begin()+pos,':');
	++pos;
	continue;
      }
      if (prevch=='/' && curch=='/' && pos>1)
	cur[pos]='%';
    }
  }

  string glue_lines_backslash(const string & s){
    int ss=s.size();
    int i=s.find('\\');
    if (i<0 || i>=ss)
      return s;
    string res,line;    
    for (i=0;i<ss;++i){
      if (s[i]!='\n'){
	line += s[i];
	continue;
      }
      int ls=line.size(),j;
      for (j=ls-1;j>=0;--j){
	if (line[j]!=' ')
	  break;
      }
      if (line[j]!='\\' || (j && line[j-1]=='\\')){
	res += line;
	res += '\n';
	line ="";
      }
      else
	line=line.substr(0,j); 
    }
    return res+line;
  }

  static void python_import(string & cur,int cs,int posturtle,int poscmath,int posmath,int posnumpy,int posmatplotlib,GIAC_CONTEXT){
    if (posmatplotlib>=0 && posmatplotlib<cs){
      cur += "np:=numpy:;xlim(a,b):=gl_x=a..b:;ylim(a,b):=gl_y=a..b:;scatter:=scatterplot:;bar:=bar_plot:;text:=legend:;";
      posnumpy=posmatplotlib;
    }
    if (posnumpy>=0 && posnumpy<cs){
      static bool alertnum=true;
      // add python numpy shortcuts
      cur += "mat:=matrix:;arange:=range:;resize:=redim:;shape:=dim:;conjugate:=conj:;full:=matrix:;eye:=identity:;ones(n,c):=matrix(n,c,1):; astype:=convert:;float64:=float:;asarray:=array:;astype:=convert:;reshape(m,n,c):=matrix(n,c,flatten(m));";
      if (alertnum){
	alertnum=false;
	alert("mat:=matrix;arange:=range;resize:=redim;shape:=dim;conjugate:=conj;full:=matrix;eye:=idn;ones(n,c):=matrix(n,c,1);reshape(m,n,c):=matrix(n,c,flatten(m));",contextptr);
      }
      return;
    }
    if (poscmath>=0 && poscmath<cs){
      // add python cmath shortcuts
      static bool alertcmath=true;      
      if (alertcmath){
	alertcmath=false;
	alert(gettext("Assigning phase, j, J and rect."),contextptr);
      }
      cur += "phase:=arg:;j:=i:;J:=i:;rect(r,theta):=r*exp(i*theta):;";
      posmath=poscmath;
    }
    if (posmath>=0 && posmath<cs){
      // add python math shortcuts
      static bool alertmath=true;      
      if (alertmath){
	alertmath=false;
	alert(gettext("Assigning log2, gamma, fabs, modf, radians and degrees."),contextptr);
      }
      cur += "log2(x):=logb(x,2):;gamma:=Gamma:;fabs:=abs:;function modf(x) local y; y:=floor(x); return x-y,y; ffunction:;radians(x):=x/180*pi:;degrees(x):=x/pi*180";
      // todo copysign, isinf, isnan, isfinite, frexp, ldexp
    }
  }


  string replace_deuxpoints_egal(const string & s){
    string res;
    int instring=0;
    for (size_t i=0;i<s.size();++i){
      char ch=s[i];
      if (i==0 || s[i-1]!='\\'){
	if (ch=='\''){
	  if (instring==2)
	    res += ch;
	  else {
	    res +='"';
	    instring=instring?0:1;
	  }
	  continue;
	}
	if (instring){
	  if (ch=='"'){
	    if (instring==1)
	      res +="\"\"";
	    else {
	      res += ch;
	      instring=0;
	    }
	  }
	  else
	    res += ch;
	  continue;
	}
	if (ch=='"'){
	  res +='"';
	  instring=2;
	  continue;
	}
      }
      switch (ch){
      case ':':
	res +="-/-";
	break;
      case '{':
	res += "{/";
	break;
      case '}':
	res += "/}";
	break;
      default:
	res += ch;
      }
    }
    return res;
  }


  // detect Python like syntax: 
  // remove """ """ docstrings and ''' ''' comments
  // cut string in lines, remove comments at the end (search for #)
  // warning don't take care of # inside strings
  // if a line of s ends with a :
  // search for matching def/for/if/else/while
  // stores matching end keyword in a stack as a vector<[int,string]>
  // int is the number of white spaces at the start of the next line
  // def ... : -> function [ffunction]
  // for ... : -> for ... do [od]
  // while ... : -> while ... do [od]
  // if ...: -> if ... then [fi]
  // else: -> else [nothing in stack]
  // elif ...: -> elif ... then [nothing in stack]
  // try: ... except: ...
  std::string python2xcas(const std::string & s_orig,GIAC_CONTEXT){
#ifdef CASCAS_DISABLE_PYTHON_RUNTIME
    return s_orig;
#else
    if (xcas_mode(contextptr)>0 && abs_calc_mode(contextptr)!=38)
      return s_orig;
    // quick check for python-like syntax: search line ending with :
    int first=0,sss=s_orig.size();
    first=s_orig.find("maple_mode");
    if (first>=0 && first<sss)
      return s_orig;
    first=s_orig.find("xcas_mode");
    if (first>=0 && first<sss)
      return s_orig;
    bool pythoncompat=python_compat(contextptr);
    bool pythonmode=false;
    first=0;
    if (sss>19 && s_orig.substr(first,17)=="add_autosimplify(")
      first+=17;
    if (s_orig[first]=='/' )
      return s_orig;
    //if (sss>first+2 && s_orig[first]=='@' && s_orig[first+1]!='@') return s_orig.substr(first+1,sss-first-1);
    if (sss>first+2 && s_orig.substr(first,2)=="@@"){
      pythonmode=true;
      pythoncompat=true;
    }
    if (s_orig[first]=='#' || (s_orig[first]=='_' && !isalpha(s_orig[first+1])) || s_orig.substr(first,4)=="from" || s_orig.substr(first,7)=="import "){
      pythonmode=true;
      pythoncompat=true;
    }
    if (pythoncompat){
      int pos=s_orig.find("{");
      if (pos>=0 && pos<sss)
	pythonmode=true;
      pos=s_orig.find("}");
      if (pos>=0 && pos<sss)
	pythonmode=true;
      pos=s_orig.find("//");
      if (pos>=0 && pos<sss)
	pythonmode=true;
      if (pos>=0 && pos<sss)
	pythonmode=true;
      pos=s_orig.find(":");
      if (pos>=0 && pos<sss-1 && s_orig[pos+1]!=';')
	pythonmode=true;
    }
    for (first=0;!pythonmode && first<sss;){
      int pos=s_orig.find(":]");
      if (pos>=0 && pos<sss){
	pythonmode=true;
	break;
      }
      pos=s_orig.find("[:");
      if (pos>=0 && pos<sss){
	pythonmode=true;
	break;
      }
      pos=s_orig.find(",:");
      if (pos>=0 && pos<sss){
	pythonmode=true;
	break;
      }
      pos=s_orig.find(":,");
      if (pos>=0 && pos<sss){
	pythonmode=true;
	break;
      }
      first=s_orig.find(':',first);
      if (first<0 || first>=sss)
	return s_orig; // not Python like
      pos=s_orig.find("lambda");
      if (pos>=0 && pos<sss)
	break;
      int endl=s_orig.find('\n',first);
      if (endl<0 || endl>=sss)
	endl=sss;
      ++first;
      if (first<endl && (s_orig[first]==';' || s_orig[first]=='=')) 
	continue; // ignore :;
      // search for line finishing with : (or with # comment)
      for (;first<endl;++first){
	char ch=s_orig[first];
	if (ch!=' '){
	  if (ch=='#')
	    first=endl;
	  break;
	}
      }
      if (first==endl) 
	break;
    }
    // probably Python-like
    string res;
    res.reserve(1.2*s_orig.size());
    res=s_orig;
    if (res.size()>18 && res.substr(0,17)=="add_autosimplify(" 
	&& res[res.size()-1]==')'
	)
      res=res.substr(17,res.size()-18);
    if (res.size()>2 && res.substr(0,2)=="@@")
      res=res.substr(2,res.size()-2);
    res=remove_comment(res,"\"\"\"",true);
    res=remove_comment(res,"'''",true);
    res=glue_lines_backslash(res);
    vector<int_string> stack;
    string s,cur;
    s.reserve(res.capacity());
    if (pythoncompat) pythonmode=true;
    for (;res.size();){
      int pos=-1;
      bool cherche=true;
      for (;cherche;){
	pos=res.find('\n',pos+1);
	if (pos<0 || pos>=int(res.size()))
	  break;
	cherche=false;
	char ch=0;
	// check if we should skip to next newline, look at previous non space
	for (int pos2=0;pos2<pos;++pos2){
	  ch=res[pos2];
	  if (ch=='#')
	    break;
	}
	if (ch=='#')
	  break;
	for (int pos2=pos-1;pos2>=0;--pos2){
	  ch=res[pos2];
	  if (ch!=' ' && ch!=9){
	    if (ch=='{' || ch=='[' || ch==',' || ch=='-' || ch=='+' ||  ch=='/')
	      cherche=true;
	    break;
	  }
	}
	for (size_t pos2=pos+1;pos2<res.size();++pos2){
	  ch=res[pos2];
	  if (ch!=' ' && ch!=9){
	    if (ch==']' || ch=='}' || ch==')')
	      cherche=true;
	    break;
	  }
	}
      }
      if (pos<0 || pos>=int(res.size())){
	cur=res; res="";
      }
      else {
	cur=res.substr(0,pos); // without \n
	res=res.substr(pos+1,res.size()-pos-1);
      }
      // detect comment (outside of a string) and lambda expr:expr
      bool instring=false,chkfrom=true;
      for (pos=0;pos<int(cur.size());++pos){
	char ch=cur[pos];
	if (ch==' ' || ch==char(9))
	  continue;
	if (!instring && pythoncompat && ch=='{' && (pos==0 || cur[pos-1]!='\\')){
	  // find matching }, counting : and , and ;
	  int c1=0,c2=0,c3=0,cs=int(cur.size()),p;
	  for (p=pos;p<cs;++p){
	    char ch=cur[p];
	    if (ch=='}' && cur[p-1]!='\\')
	      break;
	    if (ch==':')
	      ++c1;
	    if (ch==',')
	      ++c2;
	    if (ch==';')
	      ++c3;
	  }
	  if (p<cs
	      //&& c1
	      && c3==0){
	    // table initialization, replace {} by table( ) , 
	    // cur=cur.substr(0,pos)+"table("+cur.substr(pos+1,p-pos-1)+")"+cur.substr(p+1,cs-pos-1);
	    string tmp=cur.substr(0,pos);
	    tmp += "{/";
	    tmp += replace_deuxpoints_egal(cur.substr(pos+1,p-1-pos));
	    tmp += "/}";
	    tmp += cur.substr(p+1,cs-pos-1);
	    cur = tmp;
	  }
	}
	if (!instring && pythoncompat &&
	    ch=='\'' && pos<cur.size()-2 && cur[pos+1]!='\\' && (pos==0 || (cur[pos-1]!='\\' && cur[pos-1]!='\''))){ // workaround for '' string delimiters
	  static bool alertstring=true;
	  if (alertstring){
	    alert("Python compatibility, please use \"...\" for strings",contextptr);
	    alertstring=false;
	  }
	  int p=pos,q=pos+1,beg; // skip spaces
	  for (p++;p<int(cur.size());++p)
	    if (cur[p]!=' ') 
	      break;
	  if (p!=cur.size()){
	    // find matching ' 
	    beg=q;
	    for (;p<int(cur.size());++p)
	      if (cur[p]=='\'') 
		break;
	    if (p>0 && p<int(cur.size())){
	      --p;
	      // does cur[pos+1..p-1] look like a string?
	      bool str=!isalpha(cur[q]) || !isalphan(cur[p]);
	      if (p && cur[p]=='.' && cur[p-1]>'9')
		str=true;
	      if (p-q>=minchar_for_quote_as_string(contextptr))
		str=true;
	      for (;!str && q<p;++q){
		char ch=cur[q];
		if (ch=='"' || ch==' ')
		  str=true;
	      }
	      if (str){ // replace delimiters with " and " inside by \"
		string rep("\"");
		for (q=beg;q<=p;++q){
		  if (cur[q]!='"')
		    rep+=cur[q];
		  else
		    rep+="\\\"";
		}
		rep += '"';
		string tmp=cur.substr(0,pos);
		tmp += rep;
		tmp += cur.substr(p+2,cur.size()-(p+2));
		cur = tmp;
		ch=cur[pos];
	      }
	    }
	  }
	}
	if (ch=='"' && (pos==0 || cur[pos-1]!='\\')){
	  chkfrom=false;
	  instring=!instring;
	}
	if (instring)
	  continue;
	if (ch=='#'){
	  // workaround to declare local variables
	  if (cur.size()>pos+8 && (cur.substr(pos,8)=="# local " || cur.substr(pos,7)=="#local ")){
	    cur.erase(cur.begin()+pos);
	    if (cur[pos]==' ')
	      cur.erase(cur.begin()+pos);	      
	  }
	  else
	    cur=cur.substr(0,pos);
	  pythonmode=true;
	  break;
	}
	// skip from * import *
	if (chkfrom && ch=='f' && pos+15<int(cur.size()) && cur.substr(pos,5)=="from "){
	  chkfrom=false;
	  int posi=cur.find(" import ");
	  if (posi<0 || posi>=int(cur.size()))
	    posi = cur.find(" import*");
	  if (posi>pos+5 && posi<int(cur.size())){
	    int posturtle=cur.find("turtle");
	    int poscmath=cur.find("cmath");
	    int posmath=cur.find("math");
	    int posnumpy=cur.find("numpy");
	    int posmatplotlib=cur.find("matplotlib");
	    if (posmatplotlib<0 || posmatplotlib>=cur.size())
	      posmatplotlib=cur.find("pylab");
	    int cs=int(cur.size());
	    cur=cur.substr(0,pos);
	    python_import(cur,cs,posturtle,poscmath,posmath,posnumpy,posmatplotlib,contextptr);
	    pythonmode=true;
	    break;
	  }
	}
	chkfrom=false;
	// import * as ** -> **:=*
	if (ch=='i' && pos+7<int(cur.size()) && cur.substr(pos,7)=="import "){
	  int posturtle=cur.find("turtle");
	  int poscmath=cur.find("cmath");
	  int posmath=cur.find("math");
	  int posnumpy=cur.find("numpy");
	  int posmatplotlib=cur.find("matplotlib");
	  if (posmatplotlib<0 || posmatplotlib>=cur.size())
	    posmatplotlib=cur.find("pylab");
	  int cs=int(cur.size());
	  int posi=cur.find(" as ");
	  int posp=cur.find('.');
	  if (posp>=posi || posp<0)
	    posp=posi;
	  if (posi>pos+5 && posi<int(cur.size())){
	    string tmp=cur.substr(posi+4,cur.size()-posi-4);
	    tmp += ":=";
	    tmp += cur.substr(pos+7,posp-(pos+7));
	    tmp += ';';
	    cur = tmp ;
	  }
	  else
	    cur=cur.substr(pos+7,cur.size()-pos-7);
	  for (int i=0;i<pos;++i)
	    cur = ' '+cur;
	  python_import(cur,cs,posturtle,poscmath,posmath,posnumpy,posmatplotlib,contextptr);
	  pythonmode=true;
	  break;	    
	}
	if (ch=='l' && pos+6<int(cur.size()) && cur.substr(pos,6)=="lambda" && instruction_at(cur,pos,6)){
	  int posdot=cur.find(':',pos);
	  if (posdot>pos+7 && posdot<int(cur.size())-1 && cur[posdot+1]!='=' && cur[posdot+1]!=';'){
	    pythonmode=true;
	    string tmp=cur.substr(0,pos);
	    tmp += "(";
	    tmp += cur.substr(pos+6,posdot-pos-6);
	    tmp += ")->";
	    tmp += cur.substr(posdot+1,cur.size()-posdot-1);
	    cur = tmp;
	  }
	}
	if (ch=='e' && pos+4<int(cur.size()) && cur.substr(pos,3)=="end" && instruction_at(cur,pos,3)){
	  // if next char after end is =, replace by endl
	  for (size_t tmp=pos+3;tmp<cur.size();++tmp){
	    if (cur[tmp]!=' '){
	      if (cur[tmp]=='=')
		cur.insert(cur.begin()+pos+3,'l');
	      break;
	    }
	  }
	}
      }
      if (instring){
	*logptr(contextptr) << "Warning: multi-line strings can not be converted from Python like syntax"<<endl;
	return s_orig;
      }
      // detect : at end of line
      for (pos=int(cur.size())-1;pos>=0;--pos){
	if (cur[pos]!=' ' && cur[pos]!=char(9))
	  break;
      }
      if (pos<0){ 
	s+='\n';  
	continue;
      }
      if (cur[pos]!=':'){ // detect oneliner and function/fonction
	int p;
	for (p=0;p<pos;++p){
	  if (cur[p]!=' ')
	    break;
	}
	if (p<pos-8 && (cur.substr(p,8)=="function" || cur.substr(p,8)=="fonction")){
	  s += cur+'\n';
	  continue;
	}
	bool instr=false;
	for (p=pos;p>0;--p){
	  if (instr){
	    if (cur[p]=='"' && cur[p-1]!='\\')
	      instr=false;
	    continue;
	  }
	  if (cur[p]==':' && cur[p+1]!=';')
	    break;
	  if (cur[p]=='"' && cur[p-1]!='\\')
	    instr=true;	  
	}
	if (p==0){
	  // = or return expr if cond else alt_expr => ifte(cond,expr,alt_expr)
	  int cs=int(cur.size());
	  int elsepos=cur.find("else");
	  if (elsepos>0 && elsepos<cs){
	    int ifpos=cur.find("if");
	    if (ifpos>0 && ifpos<elsepos){
	      int retpos=cur.find("return"),endretpos=retpos+6;
	      if (retpos<0 || retpos>=cs){
		retpos=cur.find("=");
		endretpos=retpos+1;
	      }
	      if (retpos>=0 && retpos<ifpos){
		cur=cur.substr(0,endretpos)+" ifte("+cur.substr(ifpos+2,elsepos-ifpos-2)+","+cur.substr(endretpos,ifpos-endretpos)+","+cur.substr(elsepos+4,cs-elsepos-4)+")";
	      }
	    }
	  }
	}
	if (p>0){
	  int cs=int(cur.size()),q=4;
	  int progpos=cur.find("elif");;
	  if (progpos<0 || progpos>=cs){
	    progpos=cur.find("if");
	    q=2;
	  }
	  if (p && progpos>=0 && progpos<cs && instruction_at(cur,progpos,q)){
	    pythonmode=true;
	    string tmp=cur.substr(0,p);
#if 1
	    tmp += ":\n";
	    tmp += string(progpos+4,' ');
	    tmp += cur.substr(p+1,pos-p);
	    tmp += '\n';
	    res= tmp +res;
	    continue;
#else
	    tmp += " then ";
	    tmp += cur.substr(p+1,pos-p);
	    cur = tmp;
	    convert_python(cur,contextptr);
	    // no fi if there is an else or elif
	    for (p=0;p<int(res.size());++p){
	      if (res[p]!=' ' && res[p]!=char(9))
		break;
	    }
	    if (p<res.size()+5 && (res.substr(p,4)=="else" || res.substr(p,4)=="elif"))
	      cur += " ";
	    else
	      cur += " fi";
	    p=0;
#endif
	  }
	  progpos=cur.find("else");
	  if (p && progpos>=0 && progpos<cs && instruction_at(cur,progpos,4)){
	    pythonmode=true;
	    string tmp=cur.substr(0,p);
#if 1
	    tmp += ":\n";
	    tmp += string(progpos+4,' ');
	    tmp += cur.substr(p+1,pos-p);
	    tmp += '\n';
	    res= tmp +res;
	    continue;
#else
	    tmp += ' ';
	    tmp += cur.substr(p+1,pos-p);
	    tmp += " fi";
	    cur = tmp;
	    convert_python(cur,contextptr);
	    p=0;
#endif
	  }
	  progpos=cur.find("for");
	  if (p && progpos>=0 && progpos<cs && instruction_at(cur,progpos,3)){
	    pythonmode=true;
	    string tmp=cur.substr(0,p);
	    tmp += " do ";
	    tmp += cur.substr(p+1,pos-p);
	    cur= tmp;
	    convert_python(cur,contextptr);
	    cur += " od";
	    p=0;
	  }
	  progpos=cur.find("while");
	  if (p && progpos>=0 && progpos<cs && instruction_at(cur,progpos,5)){
	    pythonmode=true;
	    string tmp=cur.substr(0,p);
	    tmp += " do ";
	    tmp += cur.substr(p+1,pos-p);
	    cur = tmp;
	    convert_python(cur,contextptr);
	    cur += " od";
	    p=0;
	  }
	}
      }
      // count whitespaces, compare to stack
      int ws=0;
      int cs=cur.size();
      for (ws=0;ws<cs;++ws){
	if (cur[ws]!=' ' && cur[ws]!=char(9))
	  break;
      }
      if (cur[pos]==':'){
	// detect else or elif or except
	int progpos=cur.find("else");
	if (progpos>=0 && progpos<cs && instruction_at(cur,progpos,4)){
	  pythonmode=true;
	  if (stack.size()>1){ 
	    int indent=stack[stack.size()-1].decal;
	    if (ws<indent){
	      // remove last \n and add explicit endbloc delimiters from stack
	      int ss=s.size();
	      bool nl= ss && s[ss-1]=='\n';
	      if (nl)
		s=s.substr(0,ss-1);
	      while (stack.size()>1 && stack[stack.size()-1].decal>ws){
		string tmp=" ";
		tmp += stack.back().endbloc;
		tmp += ';';
		s += tmp ;
		stack.pop_back();
	      }
	      if (nl)
		s += '\n';
	    }
	  }
	  s += cur.substr(0,pos);
	  s += "\n";
	  continue;
	}
	progpos=cur.find("except");
	if (progpos>=0 && progpos<cs && instruction_at(cur,progpos,6)){
	  pythonmode=true;
	  if (stack.size()>1){ 
	    int indent=stack[stack.size()-1].decal;
	    if (ws<indent){
	      // remove last \n and add explicit endbloc delimiters from stack
	      int ss=s.size();
	      bool nl= ss && s[ss-1]=='\n';
	      if (nl)
		s=s.substr(0,ss-1);
	      while (stack.size()>1 && stack[stack.size()-1].decal>ws){
		string tmp=" ";
		tmp += stack.back().endbloc;
		tmp += ';';
		s += tmp;
		stack.pop_back();
	      }
	      if (nl)
		s += '\n';
	    }
	  }
	  s += cur.substr(0,progpos)+"then\n";
	  continue;
	}
	progpos=cur.find("elif");
	if (progpos>=0 && progpos<cs && instruction_at(cur,progpos,4)){
	  pythonmode=true;
	  if (stack.size()>1){ 
	    int indent=stack[stack.size()-1].decal;
	    if (ws<indent){
	      // remove last \n and add explicit endbloc delimiters from stack
	      int ss=s.size();
	      bool nl= ss && s[ss-1]=='\n';
	      if (nl)
		s=s.substr(0,ss-1);
	      while (stack.size()>1 && stack[stack.size()-1].decal>ws){
		string tmp=" ";
		tmp += stack.back().endbloc;
		tmp += ';';
		s += tmp;
		stack.pop_back();
	      }
	      if (nl)
		s += '\n';
	    }
	  }
	  cur=cur.substr(0,pos);
	  convert_python(cur,contextptr);
	  s += cur;
	  s += " then\n";
	  continue;
	}
      }
      if (!stack.empty()){ 
	int indent=stack.back().decal;
	if (ws<=indent){
	  // remove last \n and add explicit endbloc delimiters from stack
	  int ss=s.size();
	  bool nl= ss && s[ss-1]=='\n';
	  if (nl)
	    s=s.substr(0,ss-1);
	  while (!stack.empty() && stack.back().decal>=ws){
	    int sb=stack.back().decal;
	    string tmp=" ";
	    tmp += stack.back().endbloc;
	    tmp += ';';
	    s += tmp;
	    stack.pop_back();
	    // indent must match one of the saved indent
	    if (sb!=ws && !stack.empty() && stack.back().decal<ws){
	      return "\"Bad indentation at "+cur+"\"";
	    }
	  }
	  if (nl)
	    s += '\n';
	}
      }
      if (cur[pos]==':'){
	// detect matching programming structure
	int progpos=cur.find("if");
	if (progpos>=0 && progpos<cs && instruction_at(cur,progpos,2)){
	  pythonmode=true;
	  cur=cur.substr(0,pos);
	  convert_python(cur,contextptr);
	  s += cur;
	  s += " then\n";
	  stack.push_back(int_string(ws,"fi"));
	  continue;
	}
	progpos=cur.find("try");
	if (progpos>=0 && progpos<cs && instruction_at(cur,progpos,3)){
	  pythonmode=true;
	  cur=cur.substr(0,progpos);
	  convert_python(cur,contextptr);
	  s += cur;
	  s += "IFERR\n";
	  stack.push_back(int_string(ws,"end"));
	  continue;
	}
	progpos=cur.find("for");
	if (progpos>=0 && progpos<cs && instruction_at(cur,progpos,3)){
	  pythonmode=true;
	  // for _ -> for x_
	  cur=cur.substr(0,pos);
	  if (progpos+5<cs && cur[progpos+3]==' ' && cur[progpos+4]=='_' && cur[progpos+5]==' '){
	    cur.insert(cur.begin()+progpos+4,'x');
	  }
	  convert_python(cur,contextptr);
	  s += cur;
	  s += " do\n";
	  stack.push_back(int_string(ws,"od"));
	  continue;
	}
	progpos=cur.find("while");
	if (progpos>=0 && progpos<cs && instruction_at(cur,progpos,5)){
	  pythonmode=true;
	  cur=cur.substr(0,pos);
	  convert_python(cur,contextptr);
	  s += cur;
	  s += " do\n";
	  stack.push_back(int_string(ws,"od"));
	  continue;
	}
	progpos=cur.find("def");
	if (progpos>=0 && progpos<cs && instruction_at(cur,progpos,3)){
	  pythonmode=true;
	  //python_compat(1,contextptr); 
	  pythoncompat=true;
	  // should remove possible returned type, between -> ... and :
	  string entete=cur.substr(progpos+3,pos-progpos-3);
	  int posfleche=entete.find("->");
	  if (posfleche>0 || posfleche<entete.size())
	    entete=entete.substr(0,posfleche);
	  s += cur.substr(0,progpos);
	  s +="function";
	  s +=entete;
	  s +="\n";
	  stack.push_back(int_string(ws,"ffunction:")); // ; added later
	  continue;
	}
	// no match found, return s
	s += cur;
      }
      else {
	// normal line add ; at end
	char curpos=cur[pos];
	if (pythonmode && !res.empty() && pos>=0 && curpos!=';' && curpos!=',' && curpos!='{' && curpos!='(' && curpos!='[' && curpos!=':' && curpos!='+' && curpos!='-' && curpos!='*' && curpos!='/' && curpos!='%')
	  cur += ';';
	if (pythonmode)
	  convert_python(cur,contextptr);
	cur += '\n';
	s += cur;
      }
    }
    while (!stack.empty()){
      string tmp(" ");
      tmp += stack.back().endbloc;
      tmp += ';';
      s += tmp;
      stack.pop_back();
    }
    if (pythonmode){
      char ch;
      while ((ch=s[s.size()-1])==';'  || (ch=='\n'))
	s=s.substr(0,s.size()-1);
      // replace ;) by )
      for (int i=s.size()-1;i>=2;--i){
	if (s[i]==')' && s[i-1]=='\n' && s[i-2]==';'){
	  s.erase(s.begin()+i-2);
	  break;
	}
      }
      if (s.size()>10 && s.substr(s.size()-9,9)=="ffunction")
	s += ":;";
      else {
	int pos=s.find('\n');
	if (pos>=0 && pos<s.size())
	  s += ";";
      }
      if (0 && debug_infolevel)
	*logptr(contextptr) << "Translated to Xcas as:\n" << s << endl;
    }
    res.clear(); cur.clear();
    return string(s.begin(),s.end());
#endif
  }
  
  /* END PYTHON */
